import os
import sys
import subprocess

def compile_gui_sources(build_type):
    src_dir = "src"
    header_dir = "../"
    build_dir = ".build"

    # Create the build directory if it doesn't exist
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)
    
    # Set the src directory as the working directory
    os.chdir(src_dir)
    
    # Get list of source files in src directory
    source_files = [f for f in os.listdir() if f.endswith(".c")]

    # Compile all source files into object files
    object_files = []
    for source_file in source_files:
        object_file = os.path.join("..", build_dir, os.path.splitext(source_file)[0] + ".obj")
        object_files.append(os.path.abspath(object_file))
        if build_type == "DEBUG":
            compile_command = ["cl.exe", "/Zi", "/Od", "/c", source_file, f"/Fo{object_file}", f"/Fd{os.path.splitext(object_file)[0]}.pdb", "/I../", "/I../include"]
        else:  # Release mode
            compile_command = ["cl.exe", "/O2", "/c", source_file, f"/Fo{object_file}", "/I../", "/I../include"]
        result = subprocess.run(compile_command, capture_output=True, text=True)
        if result.returncode != 0:
            print(f"Failed to compile {source_file}.")
            print("Error Output:")
            print(result.stderr)
            print(result.stdout)
            return
        else:
            print(result.stdout)
    
    # Set the build directory as the working directory
    os.chdir("..")

    # Link object files into a static library
    lib_file = os.path.join(build_dir, "gui.lib")
    link_command = ["lib.exe", "/OUT:" + lib_file] + object_files
    result = subprocess.run(link_command, capture_output=True, text=True)
    if result.returncode == 0:
        print(result.stdout)
        print("gui lib created successfully!")
    else:
        print("Failed to create gui lib")
        print("Error Output:")
        print(result.stdout)
        print(result.stderr)

if __name__ == "__main__":
    if len(sys.argv) == 1:
        build_type = "DEBUG"  # Default to DEBUG if no arguments provided
    elif len(sys.argv) == 2 and sys.argv[1] in ["DEBUG", "RELEASE"]:
        build_type = sys.argv[1]
    else:
        print("Usage: python build.py [DEBUG | RELEASE]")
        sys.exit(1)

    compile_gui_sources(build_type)