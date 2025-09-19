# Copyright 2022 DeepMind Technologies Limited
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================
"""Interactive GUI viewer for MuJoCo."""

import abc
import atexit
import contextlib
import math
import os
import queue
import sys
import threading
import time
from typing import Callable, Optional, Tuple, Union
import weakref

import glfw
import mujoco
from mujoco import _simulate
import numpy as np
import xml.etree.ElementTree as ET

if not glfw._glfw:  # pylint: disable=protected-access
  raise RuntimeError('GLFW dynamic library handle is not available')
else:
  _simulate.set_glfw_dlhandle(glfw._glfw._handle)  # pylint: disable=protected-access

# Logarithmically spaced realtime slow-down coefficients (percent).
PERCENT_REALTIME = (
    100, 80, 66, 50, 40, 33, 25, 20, 16, 13,
    10, 8, 6.6, 5, 4, 3.3, 2.5, 2, 1.6, 1.3,
    1, 0.8, 0.66, 0.5, 0.4, 0.33, 0.25, 0.2, 0.16, 0.13,
    0.1
)

# Maximum time mis-alignment before re-sync.
MAX_SYNC_MISALIGN = 0.1

# Fraction of refresh available for simulation.
SIM_REFRESH_FRACTION = 0.7

CallbackType = Callable[[mujoco.MjModel, mujoco.MjData], None]
LoaderType = Callable[[], Tuple[mujoco.MjModel, mujoco.MjData]]

# Loader function that also returns a file path for the GUI to display.
_LoaderWithPathType = Callable[[], Tuple[mujoco.MjModel, mujoco.MjData, str]]
_InternalLoaderType = Union[LoaderType, _LoaderWithPathType]

_Simulate = _simulate.Simulate


class Handle:
  """A handle for interacting with a MuJoCo viewer."""

  def __init__(
      self,
      sim: _Simulate,
      scn: mujoco.MjvScene,
      cam: mujoco.MjvCamera,
      opt: mujoco.MjvOption,
      pert: mujoco.MjvPerturb,
  ):
    self._sim = weakref.ref(sim)
    self._scn = scn
    self._cam = cam
    self._opt = opt
    self._pert = pert

  @property
  def scn(self):
    return self._scn

  @property
  def cam(self):
    return self._cam

  @property
  def opt(self):
    return self._opt

  @property
  def perturb(self):
    return self._pert

  def close(self):
    sim = self._sim()
    if sim is not None:
      sim.exitrequest = 1

  def is_running(self) -> bool:
    sim = self._sim()
    if sim is not None:
      return sim.exitrequest < 2
    return False

  def lock(self):
    sim = self._sim()
    if sim is not None:
      return sim.lock()
    return contextlib.nullcontext()

  def sync(self):
    sim = self._sim()
    if sim is not None:
      with sim.lock():
        sim.sync()

  def __enter__(self):
    return self

  def __exit__(self, exc_type, exc_val, exc_tb):
    self.close()

def format_xml(element, indent="  ") -> str:
    """ 将 ElementTree 元素格式化为带缩进的 XML 字符串 """
    # 生成原始 XML 字符串
    raw_xml = ET.tostring(element, encoding="utf-8").decode()
    
    # 使用 minidom 美化
    reparsed = minidom.parseString(raw_xml)
    pretty_xml = reparsed.toprettyxml(indent=indent, encoding="utf-8").decode()
    
    # 去除 minidom 添加的多余空行
    pretty_xml = re.sub(r'\n\s*\n', '\n', pretty_xml)
    return pretty_xml
# Abstract base dispatcher class for systems that require UI calls to be made
# on a specific thread (e.g. macOS). This is subclassed by system-specific
# Python launcher (mjpython) to implement the required dispatching mechanism.
class _MjPythonBase(metaclass=abc.ABCMeta):

  def launch_on_ui_thread(self, model: mujoco.MjModel, data: mujoco.MjData):
    pass

# When running under mjpython, the launcher initializes this object.
_MJPYTHON: Optional[_MjPythonBase] = None

def _file_loader(path: str, terrain_path: str) -> _LoaderWithPathType:
  """Loads an MJCF model from file path."""

  def load(path=path, terrain_path=terrain_path) -> Tuple[mujoco.MjModel, mujoco.MjData, str]:
    # 解析机器人 XML
    robot_tree = ET.parse(path)
    
    # 解析地形 XML
    terrain_tree = ET.parse(terrain_path)
    terrain_root = terrain_tree.getroot()

    for child in terrain_root:
      for grandsun in child:
        robot_tree.find(".//"+child.tag).append(grandsun)
    
    # 保存新的带地形的xml
    main_with_terrain_path = path[:-8] + "main_with_terrain.xml"
    robot_tree.write(main_with_terrain_path)

    m = mujoco.MjModel.from_xml_path(main_with_terrain_path)
    d = mujoco.MjData(m)
    return m, d, path

  return load


def _reload(
    simulate: _Simulate, loader: _InternalLoaderType,
    notify_loaded: Optional[Callable[[], None]] = None
) -> Optional[Tuple[mujoco.MjModel, mujoco.MjData]]:
  """Internal function for reloading a model in the viewer."""
  try:
    load_tuple = loader()
  except Exception as e:  # pylint: disable=broad-except
    simulate.load_error = str(e)
  else:
    m, d = load_tuple[:2]

    # If the loader does not raise an exception then we assume that it
    # successfully created mjModel and mjData. This is specified in the type
    # annotation, but we perform a runtime assertion here as well to prevent
    # possible segmentation faults.
    assert m is not None and d is not None

    path = load_tuple[2] if len(load_tuple) == 3 else ''
    simulate.load(m, d, path)

    if notify_loaded:
      notify_loaded()

    return m, d


def _physics_loop(simulate: _Simulate, loader: Optional[_InternalLoaderType]):
  """Physics loop for the GUI, to be run in a separate thread."""
  m: mujoco.MjModel = None
  d: mujoco.MjData = None
  ctrl_noise = np.array([])
  reload = True

  # CPU-sim synchronization point.
  synccpu = 0.0
  syncsim = 0.0

  # Run until asked to exit.
  while not simulate.exitrequest:
    if simulate.droploadrequest:
      simulate.droploadrequest = 0
      loader = _file_loader(simulate.dropfilename)
      reload = True

    if simulate.uiloadrequest:
      simulate.uiloadrequest_decrement()
      reload = True

    if reload and loader is not None:
      result = _reload(simulate, loader)
      if result is not None:
        m, d = result
        ctrl_noise = np.zeros((m.nu,))

    reload = False

    # Sleep for 1 ms or yield, to let main thread run.
    if simulate.run != 0 and simulate.busywait != 0:
      time.sleep(0)
    else:
      time.sleep(0.001)

    with simulate.lock():
      if m is not None:
        assert d is not None
        if simulate.run:
          # Record CPU time at start of iteration.
          startcpu = glfw.get_time()

          elapsedcpu = startcpu - synccpu
          elapsedsim = d.time - syncsim

          # Inject noise.
          if simulate.ctrl_noise_std != 0.0:
            # Convert rate and scale to discrete time (Ornstein–Uhlenbeck).
            rate = math.exp(-m.opt.timestep /
                            max(simulate.ctrl_noise_rate, mujoco.mjMINVAL))
            scale = simulate.ctrl_noise_std * math.sqrt(1 - rate * rate)

            for i in range(m.nu):
              # Update noise.
              ctrl_noise[i] = (rate * ctrl_noise[i] +
                               scale * mujoco.mju_standardNormal(None))

              # Apply noise.
              d.ctrl[i] = ctrl_noise[i]

          # ===================================================================
          # observe the state
          simulator_state_pub(m, d)
          # apply control
          apply_control(m, d)

        #   motion_cap(m, d)

          # ===================================================================
            
          # Requested slow-down factor.
          slowdown = 100 / PERCENT_REALTIME[simulate.real_time_index]

          # Misalignment: distance from target sim time > MAX_SYNC_MISALIGN.
          misaligned = abs(elapsedcpu / slowdown -
                           elapsedsim) > MAX_SYNC_MISALIGN

          # Out-of-sync (for any reason): reset sync times, step.
          if (elapsedsim < 0 or elapsedcpu < 0 or synccpu == 0 or misaligned or
              simulate.speed_changed):
            # Re-sync.
            synccpu = startcpu
            syncsim = d.time
            simulate.speed_changed = False

            # Run single step, let next iteration deal with timing.
            mujoco.mj_step(m, d)

          # In-sync: step until ahead of cpu.
          else:
            measured = False
            prevsim = d.time
            refreshtime = SIM_REFRESH_FRACTION / simulate.refresh_rate
            # Step while sim lags behind CPU and within refreshtime.
            while (((d.time - syncsim) * slowdown <
                    (glfw.get_time() - synccpu)) and
                   ((glfw.get_time() - startcpu) < refreshtime)):
              # Measure slowdown before first step.
              if not measured and elapsedsim:
                simulate.measured_slowdown = elapsedcpu / elapsedsim
                measured = True

              # Call mj_step.
              mujoco.mj_step(m, d)

              # Break if reset.
              if d.time < prevsim:
                break
        else:  # simulate.run is False: GUI is paused.

          # Run mj_forward, to update rendering and joint sliders.
          mujoco.mj_forward(m, d)


def _launch_internal(
    model: Optional[mujoco.MjModel] = None,
    data: Optional[mujoco.MjData] = None,
    *,
    run_physics_thread: bool,
    loader: Optional[_InternalLoaderType] = None,
    handle_return: Optional['queue.Queue[Handle]'] = None,
) -> None:
  """Internal API, so that the public API has more readable type annotations."""
  if model is None and data is not None:
    raise ValueError('mjData is specified but mjModel is not')
  elif callable(model) and data is not None:
    raise ValueError(
        'mjData should not be specified when an mjModel loader is used')
  elif loader is not None and model is not None:
    raise ValueError('model and loader are both specified')
  elif run_physics_thread and handle_return is not None:
    raise ValueError('run_physics_thread and handle_return are both specified')

  if loader is None and model is not None:

    def _loader(m=model, d=data) -> Tuple[mujoco.MjModel, mujoco.MjData]:
      if d is None:
        d = mujoco.MjData(m)
      return m, d

    loader = _loader

  if model and not run_physics_thread:
    scn = mujoco.MjvScene(model, _Simulate.MAX_GEOM)
  else:
    scn = mujoco.MjvScene()
  cam = mujoco.MjvCamera()
  opt = mujoco.MjvOption()
  pert = mujoco.MjvPerturb()
  simulate = _Simulate(scn, cam, opt, pert, run_physics_thread)

  # Initialize GLFW if not using mjpython.
  if _MJPYTHON is None:
    if not glfw.init():
      raise mujoco.FatalError('could not initialize GLFW')
    atexit.register(glfw.terminate)

  notify_loaded = None
  if handle_return:
    notify_loaded = (
        lambda: handle_return.put_nowait(Handle(simulate, scn, cam, opt, pert)))

  side_thread = None
  if run_physics_thread:
    side_thread = threading.Thread(
        target=_physics_loop, args=(simulate, loader))
  else:
    side_thread = threading.Thread(
        target=_reload, args=(simulate, loader, notify_loaded))

  def make_exit_requester(simulate):
    def exit_requester():
      simulate.exitrequest = True
    return exit_requester

  exit_requester = make_exit_requester(simulate)
  atexit.register(exit_requester)

  side_thread.start()
  simulate.render_loop()
  atexit.unregister(exit_requester)
  side_thread.join()


def launch(model: Optional[mujoco.MjModel] = None,
           data: Optional[mujoco.MjData] = None,
           *,
           loader: Optional[LoaderType] = None) -> None:
  """Launches the Simulate GUI."""
  _launch_internal(
      model, data, run_physics_thread=True, loader=loader)


def launch_from_path(path: str, terrain_path: str) -> None:
  """Launches the Simulate GUI from file path."""
  _launch_internal(run_physics_thread=True, loader=_file_loader(path, terrain_path))


def launch_passive(model: mujoco.MjModel, data: mujoco.MjData) -> Handle:
  """Launches a passive Simulate GUI without blocking the running thread."""
  if not isinstance(model, mujoco.MjModel):
    raise ValueError(f'`model` is not a mujoco.MjModel: got {model!r}')
  if not isinstance(data, mujoco.MjData):
    raise ValueError(f'`data` is not a mujoco.MjData: got {data!r}')

  mujoco.mj_forward(model, data)
  handle_return = queue.Queue(1)

  if sys.platform != 'darwin':
    thread = threading.Thread(
        target=_launch_internal,
        args=(model, data),
        kwargs=dict(run_physics_thread=False, handle_return=handle_return),
    )
    thread.daemon = True
    thread.start()
  else:
    if not isinstance(_MJPYTHON, _MjPythonBase):
      raise RuntimeError(
          '`launch_passive` requires that the Python script be run under '
          '`mjpython` on macOS')
    _MJPYTHON.launch_on_ui_thread(model, data, handle_return)

  return handle_return.get()



import lcm
import simulator_sub_msg
import simulator_pub_msg
import yaml

last_save_time = 0.0
def motion_cap(model, data)-> None:
    global last_save_time
    file_path="motion_data.txt"

    # 获取当前时间
    current_time = time.time()

    # 判断是否到达保存数据的时间间隔（0.008333秒）
    if current_time - last_save_time >= 0.008333:
        # 记录当前时间为上次保存时间
        last_save_time = current_time

        sensor_id_jointpos = 0
        sensor_id_jointvel = model.nu
        sensor_id_accelerometer = mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_SENSOR, "baselink-baseAcc")
        sensor_id_gyro = mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_SENSOR, "baselink-gyro")
        sensor_id_framequat = mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_SENSOR, "baselink-quat")

        q = data.sensordata[sensor_id_jointpos:sensor_id_jointpos + model.nu]
        dq = data.sensordata[sensor_id_jointvel:sensor_id_jointvel + model.nu]
        sensor_id = sensor_id_accelerometer

        sensor_id = sensor_id + 3
        sensor_id = sensor_id + 3
        quat_ = data.sensordata[sensor_id: sensor_id + 4]
        quat = [quat_[1], quat_[2], quat_[3], quat_[0]]
        sensor_id = sensor_id + 4
        sensor_id = sensor_id + 1
        sensor_id = sensor_id + 1
        pos = data.sensordata[sensor_id:sensor_id + 3]
        sensor_id = sensor_id + 3

        # 将时间戳、位置、姿态、关节位置和关节速度合并为一个数据
        out_data = np.concatenate([[*pos, *quat, *q]])

        # 将数据追加到文件中
        with open(file_path, "a") as file:
            np.savetxt(file, [out_data], delimiter=" ", fmt="%.6f")  # 以6位小数格式存储
  
def simulator_state_pub(model, data)-> None:
    msg = simulator_pub_msg.simulator_pub_msg()
    msg.timestamp = int(time.time() * 1000.0)  # 当前时间戳，以毫秒为单位
    msg.motorNum = model.nu

    sensor_id_jointpos = 0
    sensor_id_jointvel = model.nu
    sensor_id_accelerometer = mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_SENSOR, "baselink-baseAcc")
    sensor_id_gyro = mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_SENSOR, "baselink-gyro")
    sensor_id_framequat = mujoco.mj_name2id(model, mujoco.mjtObj.mjOBJ_SENSOR, "baselink-quat")

    msg.q = data.sensordata[sensor_id_jointpos:sensor_id_jointpos + model.nu]
    msg.qDot = data.sensordata[sensor_id_jointvel:sensor_id_jointvel + model.nu]
    sensor_id = sensor_id_accelerometer
    msg.acc = data.sensordata[sensor_id:sensor_id + 3]
    sensor_id = sensor_id + 3
    msg.omega = data.sensordata[sensor_id:sensor_id + 3]
    sensor_id = sensor_id + 3
    msg.quat = data.sensordata[sensor_id: sensor_id + 4]
    sensor_id = sensor_id + 4
    msg.lf_touch = data.sensordata[sensor_id:sensor_id + 1]
    sensor_id = sensor_id + 1
    msg.rf_touch = data.sensordata[sensor_id:sensor_id + 1]
    sensor_id = sensor_id + 1
    msg.pos = data.sensordata[sensor_id:sensor_id + 3]
    sensor_id = sensor_id + 3
    msg.vel = data.sensordata[sensor_id:sensor_id + 3]

    lc.publish("msg_simulator_to_controller", msg.encode())
    
def lcm_message_handler_loop()-> None:
    if not lc:
        print("LCM not initialized!")
        return
    subscription = lc.subscribe("msg_controller_to_simulator", lcm_message_handler)
    try:
        while True:
            lc.handle()
            # time.sleep(0.001)
    except KeyboardInterrupt:
        print("Shutting down...")
    finally:
        lc.unsubscribe(subscription)

def lcm_message_handler(channel, data):
    global lcm_sub_msg
    lcm_sub_msg = lcm_sub_msg_decoder.decode(data)

def apply_control(model, data) -> None:

    sensor_id_jointpos = 0
    sensor_id_jointvel = model.nu
    if lcm_sub_msg is None:
        data.ctrl[:] = np.zeros(model.nu)
    else:
        if (lcm_sub_msg.started == 0):
            data.ctrl[:] = np.zeros(model.nu)
        else:
            kp = lcm_sub_msg.q_kp
            kd = lcm_sub_msg.q_kd
            data.ctrl[:] = kp * (lcm_sub_msg.qCmd - data.sensordata[sensor_id_jointpos:sensor_id_jointpos + model.nu])  \
                         + kd * (lcm_sub_msg.qDotCmd - data.sensordata[sensor_id_jointvel:sensor_id_jointvel + model.nu]) \
                         + lcm_sub_msg.torqCmd

if __name__ == '__main__':
  from absl import app  # pylint: disable=g-import-not-at-top
  from absl import flags  # pylint: disable=g-import-not-at-top

  _MJCF_PATH = flags.DEFINE_string('mjcf', None, 'Path to MJCF file.')
  _TERRAIN_PATH = flags.DEFINE_string('terrain', None, 'Path to TERRAIN file.')
  global lc
  global lcm_sub_msg_decoder, lcm_sub_msg
  global lcm_sub_thread
  lc = lcm.LCM()    
  
  lcm_sub_msg_decoder = simulator_sub_msg.simulator_sub_msg()
  lcm_sub_msg = None
  lcm_sub_thread = threading.Thread(target=lcm_message_handler_loop)
  lcm_sub_thread.start()
  def main(argv) -> None:
    del argv
    if _MJCF_PATH.value is not None and _TERRAIN_PATH.value is not None:
      launch_from_path(os.path.expanduser(_MJCF_PATH.value), os.path.expanduser(_TERRAIN_PATH.value))
    else:
      launch()

  app.run(main)
