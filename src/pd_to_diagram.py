import subprocess
import shlex
import os

# 当前文件所在目录
DIRNOW = os.path.dirname(os.path.abspath(__file__))
EXEPATH = os.path.join(DIRNOW, "line_checker.out")

def run_command(command: str, directory: str = '.') -> tuple[int, str, str]:
    """
    在指定目录下执行 shell 命令

    参数:
    command (str): 要执行的 shell 命令
    directory (str): 执行命令的工作目录 (默认: 当前目录)

    返回:
    tuple: (返回码, 标准输出, 标准错误)
    """
    try:
        # 检查目录是否存在
        if not os.path.exists(directory):
            raise FileNotFoundError(f"目录不存在: {directory}")
        if not os.path.isdir(directory):
            raise NotADirectoryError(f"不是有效的目录: {directory}")

        # 直接传递命令字符串，使用 shell=True
        result = subprocess.run(
            command,  # 直接使用字符串，不需要拆分
            shell=True,
            cwd=directory,
            capture_output=True,
            text=True,
            check=False
        )

        return result.returncode, result.stdout, result.stderr

    except Exception as e:
        return 1, "", str(e)

# 如果当前没有找到可执行文件
# 那么就自动重新编译一个，需要保证 bash 和 g++ 可用
def auto_compile():
    if not os.path.isfile(EXEPATH): # 确保可执行文件被生成出来了
        print("compiling ...")
        returncode, stdout, stderr = run_command("bash compile.sh", DIRNOW)
        if returncode != 0:
            print(f"编译失败: {stderr}")
        assert os.path.isfile(EXEPATH)

if __name__ == "__main__":    
    auto_compile()