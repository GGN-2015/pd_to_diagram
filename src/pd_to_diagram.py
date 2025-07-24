import subprocess
import os
import time
import tempfile
import call_line_checker
import uuid
import math
import random
import json
from sa import SimulatedAnnealing # 模拟退火程序

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
def auto_compile(force_compile=False):
    if not os.path.isfile(call_line_checker.LIBPATH) or force_compile: # 确保可执行文件被生成出来了
        print("compiling ...")
        returncode, stdout, stderr = run_command("bash compile.sh", call_line_checker.DIRNOW)
        if returncode != 0:
            print(f"编译失败: {stderr}")
        assert os.path.isfile(call_line_checker.LIBPATH)
    # 保证可执行权限
    run_command("chmod +x '%s'" % call_line_checker.LIBPATH)

# 模拟退火的目标函数
# solution 结构示例：
#     {"grid_size":6,"crossing_number":3,"pos_list":[[4,4],[5,2],[2,2]],"direction_list":[2,1,0],"pd_code":[[6,4,1,3],[4,2,5,1],[2,6,3,5]]}
#     其中 pos_list 和 direction_list 核心参与优化部分
def objective_function(solution:dict) -> float:
    temp_dir = tempfile.gettempdir()
    full_path = os.path.join(temp_dir, f"tmp_{uuid.uuid4().hex}.txt")

    # 构建随机输入文件
    with open(full_path, "w") as fp:
        fp.write("%d\n" % solution["grid_size"])       # 网格大小
        fp.write("%d\n" % solution["crossing_number"]) # 交叉点数目

        for i in range(solution["crossing_number"]): # 依次输出：位置，朝向，pd_code
            fp.write(" ".join(list(map(str, solution["pos_list"][i]))) + "\n")
            fp.write("%d\n" % solution["direction_list"][i])
            fp.write(" ".join(list(map(str, solution["pd_code"][i]))) + "\n")

    lc = call_line_checker.LineChecker()
    answer_now = lc.call(full_path)

    # 删除随机输入文件
    os.remove(full_path)
    return answer_now

def min_manhattan_distance(points: list[list[int | float]]) -> float:
    """
    计算二维点序列中两两之间的曼哈顿距离的最小值
    
    参数:
        points: 二维点序列，格式为 [[x1, y1], [x2, y2], ..., [xn, yn]]
    
    返回:
        所有点对之间的最小曼哈顿距离
    
    异常:
        ValueError: 当输入点数量少于 2 时抛出
    """
    # 检查输入合法性
    if len(points) < 2:
        raise ValueError("至少需要 2 个点才能计算距离")
    
    min_dist = float('inf')  # 初始化最小值为无穷大
    n = len(points)
    
    # 遍历所有点对（i < j 避免重复计算）
    for i in range(n):
        x1, y1 = points[i]
        for j in range(i + 1, n):
            x2, y2 = points[j]
            # 计算曼哈顿距离：|x1 - x2| + |y1 - y2|
            dist = abs(x1 - x2) + abs(y1 - y2)
            # 更新最小值
            if dist < min_dist:
                min_dist = dist
                # 提前退出（若找到距离为 0 的点对，可直接返回）
                if min_dist == 0:
                    return 0.0
    
    return float(min_dist)

# 用于生成一个新的邻居
neighbor_generator_called_time = 0
def neighbor_generator(solution_now:dict) -> dict:
    global neighbor_generator_called_time
    assert solution_now is not None
    neighbor_generator_called_time += 1                 # 统计函数被调用了多少次
    N = solution_now["grid_size"]

    while True:

        # 随机修改一个交叉点的位置，修改他的朝向和位置
        new_solution = json.loads(json.dumps(solution_now)) # 深拷贝一下
        pos_to_change = random.randint(0, new_solution["crossing_number"] - 1)
        new_solution["direction_list"][pos_to_change] = random.randint(0, 3) # 生成一个随机方向
        new_solution["pos_list"][pos_to_change] = [random.randint(2, N-1), random.randint(2, N-1)]

        # 找到了一个合法的解
        if min_manhattan_distance(new_solution["pos_list"]) >= 3:
            return new_solution

def gen_init(pd_code) -> dict: # 生成初始解
    cnt = 0
    begin_time = time.time()
    while True:
        cnt += 1
        grid_size = 10 * len(pd_code) + 1
        initial_solution = {
            "grid_size": grid_size,
            "crossing_number": len(pd_code),
            "pos_list": [],
            "direction_list": [],
            "pd_code": pd_code
        }
        for _ in range(len(pd_code)):
            N = initial_solution["grid_size"]
            initial_solution["pos_list"].append([random.randint(2, N-1), random.randint(2, N-1)])
            initial_solution["direction_list"].append(random.randint(0, 3))
        
        # 遇到可行解的时候就返回，感觉初始可行解也不太好找
        if min_manhattan_distance(initial_solution["pos_list"]) >= 3 and objective_function(initial_solution) < 2 * (grid_size - 1) ** 2:
            print("用时 %13.7f, 已检查 %d 个初始解, 找到了初始可行解" % (time.time() - begin_time, cnt))
            return initial_solution
        else:
            if cnt % 100 == 0:
                print("用时 %13.7f, 已检查 %d 个初始解, 尚未找到初始可行解" % (time.time() - begin_time, cnt))

# 给定一个 pd_code 求一个扭结图出来
def solve_diagram_for_pd_code(pd_code:list, seed=42):
    random.seed(seed)

    initial_solution = gen_init(pd_code) # 生成初始解
    sa = SimulatedAnnealing(objective_function, neighbor_generator, initial_temperature= 4 * len(pd_code)* len(pd_code), seed=None)
    sa.run(initial_solution)

if __name__ == "__main__":    
    auto_compile(True)
    # solve_diagram_for_pd_code([[2, 8, 3, 7], [4, 10, 5, 9], [6, 2, 7, 1], [8, 4, 9, 3], [10, 6, 1, 5]])
    # solve_diagram_for_pd_code([[6, 1, 7, 2], [10, 7, 5, 8], [4, 5, 1, 6], [2, 10, 3, 9], [8, 4, 9, 3]])
    solve_diagram_for_pd_code([[6,1,7,2],[3,13,4,12],[7,21,8,20],[19,5,20,10],[13,19,14,22],[21,11,22,18],[17,15,18,14],[9,17,10,16],[15,9,16,8],[2,5,3,6],[11,1,12,4]])
