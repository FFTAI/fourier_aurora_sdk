import os
import pinocchio as pin

def print_urdf_joint_and_link_names(urdf_file):
    """加载 URDF 文件并打印所有关节和连杆的名称"""
    if not os.path.isfile(urdf_file):
        print(f"File {urdf_file} not found.")
        return

    # 创建模型
    model = pin.buildModelFromUrdf(urdf_file)

    # 打印关节信息
    print(f"Number of joints: {model.njoints}")
    print("Joint IDs and names:")
    for joint_id in range(model.njoints):
        joint_name = model.names[joint_id]  # 获取关节名称
        print(f"  Joint ID {joint_id}: {joint_name}")

    # 打印连杆信息
    print(f"\nNumber of frames (links): {model.nframes}")
    print("Link IDs and names:")
    for frame_id in range(model.nframes):
        frame_name = model.frames[frame_id].name  # 通过 frames 获取连杆名称
        print(f"  Link ID {frame_id}: {frame_name}")

if __name__ == "__main__":
    # 替换为实际的 URDF 文件路径
    urdf_file_path = './resources/GR2T2V2/urdf/GR2T2v2.urdf'
    print_urdf_joint_and_link_names(urdf_file_path)
