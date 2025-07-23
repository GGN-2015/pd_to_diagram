import math
import random
from typing import Callable, Any, Tuple, Optional

class SimulatedAnnealing:
    """
    模拟退火算法优化器，用于最小化目标函数
    """
    def __init__(
        self,
        objective_function: Callable[[Any], float],
        neighbor_function: Callable[[Any], Any],
        initial_temperature: float = 1000.0,
        final_temperature: float = 0.001,
        cooling_rate: float = 0.95,
        equilibrium_iterations: int = 200,
        seed:Optional[int] = None
    ):
        """
        初始化模拟退火优化器
        
        参数:
            objective_function: 目标函数，输入为优化变量，输出为目标值（需最小化）
            neighbor_function: 邻域生成函数，输入当前解，返回其邻域内的一个随机解
            initial_temperature: 初始温度
            final_temperature: 终止温度
            cooling_rate: 降温速率
            equilibrium_iterations: 每个温度下的平衡迭代次数
            seed: 随机数种子，用于结果可复现
        """
        self.objective_function = objective_function
        self.neighbor_function = neighbor_function
        self.initial_temperature = initial_temperature
        self.final_temperature = final_temperature
        self.cooling_rate = cooling_rate
        self.equilibrium_iterations = equilibrium_iterations
        
        if seed is not None:
            random.seed(seed)
        
        # 优化结果
        self.best_solution = None
        self.best_value = float('inf')
        self.current_solution = None
        self.current_value = float('inf')
        self.iterations = 0
        self.temperature = initial_temperature
        
        # 记录优化过程
        self.history = {
            'iterations': [],
            'temperature': [],
            'current_value': [],
            'best_value': []
        }
    
    def initialize(self, initial_solution: Any = None) -> None:
        """
        初始化优化过程
        
        参数:
            initial_solution: 初始解，若为None则随机生成
        """
        if initial_solution is None:
            # 若未提供初始解，尝试通过邻域函数生成
            self.current_solution = self.neighbor_function(None)
        else:
            self.current_solution = initial_solution
            
        self.current_value = self.objective_function(self.current_solution)
        self.best_solution = self.current_solution
        self.best_value = self.current_value
        self.temperature = self.initial_temperature
        self.iterations = 0
        
        # 初始化历史记录
        self.history = {
            'iterations': [0],
            'temperature': [self.temperature],
            'current_value': [self.current_value],
            'best_value': [self.best_value]
        }
    
    def accept_criterion(self, delta: float) -> bool:
        """
        接受准则，决定是否接受邻域解
        
        参数:
            delta: 目标函数变化量
            
        返回:
            是否接受新解
        """
        if delta < 0:
            # 若新解更优，直接接受
            return True
        else:
            # 若新解更差，以一定概率接受
            probability = math.exp(-delta / self.temperature)
            return random.random() < probability
    
    def update_temperature(self) -> None:
        """
        更新温度（降温）
        """
        self.temperature *= self.cooling_rate
    
    def run(self, initial_solution: Any = None) -> Tuple[Any, float]:
        """
        执行模拟退火优化
        
        参数:
            initial_solution: 初始解，若为None则利用neighbor_function随机生成
            
        返回:
            最优解及其目标函数值
        """
        self.initialize(initial_solution)
        
        while self.temperature > self.final_temperature:
            for _ in range(self.equilibrium_iterations):
                # 生成邻域解
                neighbor = self.neighbor_function(self.current_solution)
                neighbor_value = self.objective_function(neighbor)
                
                # 计算目标函数变化量
                delta = neighbor_value - self.current_value
                
                # 判断是否接受新解
                if self.accept_criterion(delta):
                    self.current_solution = neighbor
                    self.current_value = neighbor_value
                    
                    # 更新最优解
                    if self.current_value < self.best_value:
                        self.best_solution = self.current_solution
                        self.best_value = self.current_value
            
            # 记录当前迭代信息
            self.iterations += 1
            self.history['iterations'].append(self.iterations)
            self.history['temperature'].append(self.temperature)
            self.history['current_value'].append(self.current_value)
            self.history['best_value'].append(self.best_value)
            
            # 降温
            self.update_temperature()
        
        return self.best_solution, self.best_value
    
    def get_history(self) -> dict:
        """
        获取优化历史记录
        
        返回:
            包含优化过程中温度、当前解和最优解变化的字典
        """
        return self.history


# 使用示例
if __name__ == "__main__":
    # 示例：最小化二维Rastrigin函数
    def rastrigin_function(x: list) -> float:
        """Rastrigin测试函数，全局最小值在(0,0)处，值为0"""
        A = 10
        return float(A * 2 + sum([(xi**2 - A * math.cos(2 * math.pi * xi)) for xi in x]))
    
    def neighbor_generator(x: list) -> list:
        """生成邻域解：在当前解的每个维度上添加一个小的随机扰动"""
        if x is None:
            # 生成初始解
            return [random.uniform(-5.12, 5.12) for _ in range(2)]
        # 在当前解基础上生成邻域解
        return [xi + random.uniform(-1.0, 1.0) for xi in x]
    
    # 创建优化器实例
    sa = SimulatedAnnealing(
        objective_function=rastrigin_function,
        neighbor_function=neighbor_generator,
    )
    
    # 执行优化
    best_solution, best_value = sa.run()
    
    print(f"优化结果:")
    print(f"最优解: {best_solution}")
    print(f"目标函数值: {best_value:.6f}")