import sys
import subprocess
import os

def get_latest_windows_kit_include_path():
    windows_kit_include_path = r"C:\Program Files (x86)\Windows Kits\10\Include"
    
    # Get all subdirectories within the Windows Kits include directory
    subdirectories = [d for d in os.listdir(windows_kit_include_path) if os.path.isdir(os.path.join(windows_kit_include_path, d))]
    
    # Filter out directories that don't start with a version number
    subdirectories = [d for d in subdirectories if d.split(".")[0].isdigit()]
    
    # Sort the directories based on version number
    sorted_subdirectories = sorted(subdirectories, key=lambda x: tuple(map(int, x.split("."))), reverse=True)
    
    # Choose the directory with the highest version number
    if sorted_subdirectories:
        return os.path.join(windows_kit_include_path, sorted_subdirectories[0])
    else:
        return None


def compile_d3d_example(build_type):
    source_file = "sample_d3d11.c"
    output_file = "sample_d3d11.exe"
    build_dir = "../sample/.build"
    gui_lib_dir = "../gui"

    # Get the latest Windows Kits include path
    include_path = get_latest_windows_kit_include_path()
    if include_path is None:
        print("Error: Failed to locate the latest version of the Windows Kits include directory.")
        return
    gui_include_path = os.path.abspath(os.path.join(gui_lib_dir, "include"))

    # Compile the GUI library
    os.chdir(gui_lib_dir)
    compile_gui_lib_command = ["python3", "build.py", build_type]
    result = subprocess.run(compile_gui_lib_command, capture_output=True, text=True)
    print(result.stdout)
    print(result.stderr)
    
    
    # Create the build directory if it doesn't exist
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)
    
    # Set the build directory as the working directory
    os.chdir(build_dir)
    
    # Compile the source file
    if build_type == "DEBUG":
        compile_command = ["cl.exe", "/Zi", "/I", include_path,"/I", gui_include_path, os.path.join("..", source_file), "/link", "/SUBSYSTEM:WINDOWS", f"/OUT:{output_file}"]
    else:  # Release mode
        compile_command = ["cl.exe", "/O2", "/I", include_path,"/I", gui_include_path, os.path.join("..", source_file), "/link", "/SUBSYSTEM:WINDOWS", f"/OUT:{output_file}"]
    result = subprocess.run(compile_command, capture_output=True, text=True)
    if result.returncode == 0:
        print(result.stdout)
        print("D3D11 example compiled successfully!")
    else:
        print("Failed to compile D3D11 example.")
        print("Error Output:")
        print(result.stderr)
        print(result.stdout)

if __name__ == "__main__":
    if len(sys.argv) == 1:
        build_type = "DEBUG"  # Default to DEBUG if no arguments provided
    elif len(sys.argv) == 2 and sys.argv[1] in ["DEBUG", "RELEASE"]:
        build_type = sys.argv[1]
    else:
        print("Usage: python build.py [DEBUG | RELEASE]")
        sys.exit(1)

    compile_d3d_example(build_type)