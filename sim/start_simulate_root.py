import os
import sys
import subprocess

def list_robot_names(base_directory):
    try:
        # 列出资源目录下的所有子目录
        robot_names = [name for name in os.listdir(base_directory) if os.path.isdir(os.path.join(base_directory, name))]
        # 移除名为 'terrain' 的元素
        robot_names = [name for name in robot_names if name.lower() != 'terrains']
        robot_names.sort()  # 或者使用 sorted(robot_names) 来返回一个新排序的列表
        return robot_names
    except Exception as e:
        print("Error listing robot names:", e)
        return []

def list_terrain_names(base_directory):
    try:
        # 列出资源目录下的所有子目录
        terrain_names = [name for name in os.listdir(base_directory) if name.endswith('.xml')]
        terrain_names.sort()  # 或者使用 sorted(terrain_names) 来返回一个新排序的列表
        return terrain_names
    except Exception as e:
        print("Error listing terrain names:", e)
        return []

def start_viewer():
    # 设置目标目录及其子目录
    base_viewer_directory = os.path.abspath(os.path.join(os.path.dirname(__file__), '../sim/mujoco/'))
    resources_directory = '/usr/local/resources'  # 资源目录的绝对路径
    viewer_path = os.path.join(base_viewer_directory, 'viewer.py')  # viewer.py 的完整路径
  
    # 列出可用的机器人名称
    robot_names = list_robot_names(resources_directory)
    if robot_names:
        print("\033[31m可选择的机器人名称: \033[0m")
        for idx, name in enumerate(robot_names):
            print(f"\033[31m{idx}: {name}\033[0m")  # 红色字体输出
    else:
        print("没有找到任何机器人名称。")
        return

    # 输入启动的机器人名称，允许输入数字以选择
    robot_name_input = input("请输入启动的机器人序号: ")
    
    # 处理输入
    if robot_name_input.isdigit():
        index = int(robot_name_input)
        if 0 <= index < len(robot_names):
            robot_name = robot_names[index].upper()  # 根据索引选择并转换为大写
        else:
            print("无效的选择，请输入有效的序列号。")
            return
    else:
        print("无效的输入，请输入数字。")
        return

    xml_path = os.path.join(resources_directory, robot_name, 'mjcf', 'main.xml')

    # 列出可用的地形名称
    terrain_names = list_terrain_names(os.path.join(resources_directory, '../terrains'))
    if terrain_names:
        print("\033[31m可选择地形名称: \033[0m")
        for idx, name in enumerate(terrain_names):
            print(f"\033[31m{idx}: {name[:-4].upper()}\033[0m")  # 红色字体输出
    else:
        print("没有找到任何地形名称。")
        return
    
    # 输入启动的地形名称，允许输入数字以选择
    terrain_name_input = input("请输入启动的地形序号: ")

    # 处理输入
    if terrain_name_input.isdigit():
        index = int(terrain_name_input)
        if 0 <= index < len(terrain_names):
            terrain_name = terrain_names[index]
        else:
            print("无效的选择，请输入有效的序列号。")
            return
    else:
        print("无效的输入，请输入数字。")
        return
    terrain_path = os.path.join(resources_directory, '../terrains', terrain_name)

    # 确保路径存在
    if not os.path.exists(viewer_path):
        print("Viewer script not found at:", viewer_path)
        return

    if not os.path.exists(xml_path):
        print("XML file not found at:", xml_path)
        return
    if not os.path.exists(terrain_path):
        print("XML file not found at:", terrain_path)
        return

    try:
        # 确保使用 Python 3 来运行 viewer.py
        python_executable = '/usr/bin/python3'  # 根据你的系统路径调整
        process = subprocess.Popen(
            [python_executable, viewer_path, '--mjcf', xml_path, '--terrain', terrain_path],
            stdout=subprocess.PIPE,  # 重定向标准输出
            stderr=subprocess.PIPE,   # 重定向标准错误
            text=False                # 以字节模式处理输出
        )

        print("Viewer started with PID:", process.pid)

        # 实时输出 viewer 的消息
        while True:
            output = process.stdout.readline()
            if output == b'' and process.poll() is not None:
                break
            if output:
                print(output.decode('latin-1').strip())  # 使用 latin-1 解码

        # 获取错误信息
        stderr_output = process.stderr.read()
        if stderr_output:
            print(stderr_output.decode('latin-1').strip())

    except KeyboardInterrupt:
        print("Terminating viewer...")
        process.terminate()  # 发送终止信号给子进程
        process.wait()       # 等待子进程结束
        print("Viewer terminated.")

    except Exception as e:
        print("Error starting viewer:", e)
        if 'utf-8' in str(e):
            print("Output may contain non-UTF-8 characters.")

if __name__ == "__main__":
    start_viewer()
