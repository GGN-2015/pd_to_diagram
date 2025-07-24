import ctypes
import os
from typing import Tuple

# 当前文件所在目录
DIRNOW = os.path.dirname(os.path.abspath(__file__))
LIBPATH = os.path.join(DIRNOW, "line_checker.so")

class LineChecker:
    def __init__(self):
        """
        初始化 LineChecker 实例，加载动态链接库
        
        参数:
            lib_path: 编译好的 C++ 动态链接库路径（如 .so, .dll, .dylib）
        """
        # 加载动态链接库
        if not os.path.exists(LIBPATH):
            raise FileNotFoundError(f"动态链接库不存在: {LIBPATH}")
        
        self.lib = ctypes.CDLL(LIBPATH)
        
        # 定义 call_main 函数的参数和返回值类型
        # 函数原型：extern int call_main(char* filename)
        self.lib.call_main.argtypes = [ctypes.c_char_p]  # 参数为字符串指针
        self.lib.call_main.restype = ctypes.c_double     # 返回值为 double

    def call(self, file_path: str) -> float:
        """
        调用 C++ 中的 call_main 函数
        
        参数:
            file_path: 要传入的文件路径
        
        返回:
            元组 (返回码, 状态信息)
            - 返回码 0: 成功
            - 返回码 1: 文件打开失败
            - 其他返回码: 未知错误
        """
        # 将 Python 字符串转换为 C 字符串（bytes 类型）
        c_file_path = ctypes.c_char_p(file_path.encode('utf-8'))
        
        # 调用 C++ 函数
        ret_code = self.lib.call_main(c_file_path)
        return float(ret_code)

if __name__ == "__main__":
    lc = LineChecker()