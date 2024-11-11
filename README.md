# FPGA_r_op

The main workspace is in `tranmute.` usage after `make`:

```
./build ../benchmark/case_1.nodes ../benchmark/case_1.nets ../benchmark/case_1.timing output/case_1.nodes.out
```

在`info/sclsort.txt`中打出了Tiles各自的属性

## haotong directory说明

该项目的文件结构来源于 `transmute` 文件夹，并在 `haotong` 文件夹中进行了复制和修改。主要更新如下：

### 目录结构

- `solver/`：在此子文件夹中增加了用于计算线长的 `wirelength.cpp` 文件。
- `global.cpp` 和 `global.h`：引入了全局变量和函数。
- `rsmt.cpp` 和 `rsmt.h`：引入了相关的 RSMT（Rectilinear Steiner Minimum Tree）计算模块。

### 新增文件

- **wirelength.cpp**：用于计算线长。
- **global.cpp** 和 **global.h**：用于定义和声明全局变量和函数。
- **rsmt.cpp** 和 **rsmt.h**：包含 RSMT 计算的相关实现和声明。

此项目主要用于线长计算和全局变量的管理，所有修改和新增文件均位于 `haotong` 文件夹下。
