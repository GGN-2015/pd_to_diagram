import ctypes
import os
import numpy as np
import functools
import warnings

# 当前文件所在目录
DIRNOW = os.path.dirname(os.path.abspath(__file__))
LIBPATH = os.path.join(DIRNOW, "line_checker.so")

def warn_first_call(message: str):
    """
    装饰器：函数第一次被调用时输出警告信息，后续调用不输出。
    
    参数:
        message: 首次调用时要显示的警告信息
    """
    def decorator(func):
        @functools.wraps(func)  # 保留原函数的元信息（如名称、文档字符串）
        def wrapper(*args, **kwargs):
            # 检查函数是否被调用过（通过自定义属性 `_has_been_called` 标记）
            if not hasattr(wrapper, '_has_been_called'):
                # 首次调用：输出警告
                warnings.warn(message, UserWarning)
                wrapper._has_been_called = True  # 标记为已调用
            # 执行原函数
            return func(*args, **kwargs)
        return wrapper
    return decorator

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
        
        # 定义 call_main_with_array 函数的参数和返回值类型
        # 函数原型：extern double call_main_with_array(int* arr, int cnt)
        self.lib.call_main_with_array.argtypes = [
            np.ctypeslib.ndpointer(dtype=np.int32, flags='C_CONTIGUOUS'),
            ctypes.c_int
        ]
        self.lib.call_main_with_array.restype = ctypes.c_double

    @warn_first_call("LineChecker.call is deprecated since it sometimes trigger OS Error: Open Too Many Files")
    def call(self, file_path: str) -> float:
        """
        调用 C++ 中的 call_main 函数
        
        参数:
            file_path: 要传入的文件路径
        
        返回:
            处理结果的浮点数表示
        """
        assert os.path.isfile(file_path) # 检查文件是否存在

        # 将 Python 字符串转换为 C 字符串（bytes 类型）
        c_file_path = ctypes.c_char_p(file_path.encode('utf-8'))
        
        # 调用 C++ 函数
        ret_code = self.lib.call_main(c_file_path)
        return float(ret_code)

    def call_with_array(self, arr: np.ndarray) -> float:
        """
        调用 C++ 中的 call_main_with_array 函数，处理 numpy 数组
        
        参数:
            arr: 输入的 numpy 数组，必须是 int32 类型且连续存储
        
        返回:
            处理结果的浮点数表示
        """
        # 确保数组是 int32 类型
        arr = np.ascontiguousarray(arr, dtype=np.int32)
        
        # 调用 C++ 函数
        result = self.lib.call_main_with_array(arr, len(arr))
        return float(result)

if __name__ == "__main__":
    lc = LineChecker()
    
    # 示例：调用 call_with_array 方法
    try:
        # 创建示例数组
        arr = np.array([51,5,11,36,2,6,1,7,2,13,37,1,10,7,5,8,12,34,3,4,5,1,6,14,39,2,2,10,3,9,16,37,3,8,4,9,3], dtype=np.int32)
        result = lc.call_with_array(arr)
        print(f"call_with_array 方法返回结果: {result}")
    except Exception as e:
        print(f"调用 call_with_array 方法失败: {e}")
