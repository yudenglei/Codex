# CAEFrame 多版本分类对比与最终综合架构（v1）

## 1. 文件分类（按来源与版本）

### 1.1 ChatGPT 系列
- `CAEFrame-ChatGPT-v1.md`：**全量替换型**（完整工程，强调 undo/redo + Cap'n Proto + zstd + 参数表达式）。
- `CAEFrame-ChatGPT-v2.md`：**全量替换型**（扩展实体覆盖、补齐 save/load、加入 tests/bench）。
- `CAEFrame-ChatGPT-v3.md`：**专题扩展型**（2D/3D 视图，偏 UI/渲染，不是纯数据内核替换）。

### 1.2 Gemini 系列
- `CAEFrame-Gemini-v1.md`：**架构原型型**（Parametric Value 8-byte tagged handle 思路清晰）。
- `CAEFrame-Gemini-v2.md`：**全量骨架型**（Entity-Index + Delta/Command）。
- `CAEFrame-Gemini-v3.md`：**增量优化型**（StringPool transparent lookup、Layer 索引、visit 派发）。
- `CAEFrame-Gemini-v4.md`：**专题扩展型**（Cap'n Proto + 2D/3D 渲染模块化）。

### 1.3 Deepseek 系列
- `CAEFrame-Deepseek-v1.md`：**架构草案型**（高性能低内存核心框架）。
- `CAEFrame-Deepseek-v2.md`：**全量替换型**（模块化完整代码，包含并发、LayerIndex、QuadTree 倾向）。

### 1.4 Grok 系列
- `CAEFrame-Grok-v10.md`：**全量替换型**（大而全基线版本）。
- `CAEFrame-Grok-v11.md`：**增量迭代型**（未单独打开全文，但从序列位置与 v12 引用可判定为 v10 的演进）。
- `CAEFrame-Grok-v12.md`：**增量迭代型**（明确写明“直接替换 v11.0”，补齐 BatchChange + per-layer lock + QuadTree+LayerIndex）。
- `CAEFrame-Grok-v13.md`：**增量优化型**（明确写明“基于 v12.0 增量更新”，重点是 undo 内存模型优化与 visit/X-macro分析）。
- `CAEFrame-Grok-v14.md`：**全量替换型**（混合架构成形：全局权威 + 单层连续索引）。
- `CAEFrame-Grok-v15.md`：**全量替换型**（继续扩展依赖/功能）。
- `CAEFrame-Grok-v16.md`：**全量替换型**（“最终完整生产级代码”）。
- `CAEFrame-Grok-v17.md`：**全量替换型（当前 Grok 收敛版）**。

### 1.5 豆包系列
- `CAEFrame-doubao-v1.md`：**全量替换型**（强调 Undo 去序列化、Cap'n Proto 仅用于持久化、参数池重构）。
- `CAEFrame-doubao-v2.md`：**全量替换型**（用 ExprTk 替换自研表达式解析）。

---

## 2. 增量与替换识别结论

- **增量型（明确演进链）**：
  - `Grok v11 -> v12 -> v13`（v12/v13 文中明确出现“替换/增量”措辞）。
  - `Gemini v3` 对 `v2` 的性能点增强。
- **全量替换型（可独立落地）**：
  - ChatGPT v1/v2、Deepseek v2、Grok v10/v14/v15/v16/v17、doubao v1/v2。
- **专题扩展型（非内核替换）**：
  - ChatGPT v3、Gemini v4（偏 2D/3D 视图层）。

---

## 3. 横向对比（性能/内存/工程化）

### 3.1 一致高价值共识
1. **Handle/ID 化对象关系**：避免重对象嵌套，支持轻量引用。
2. **字符串池**：名称/描述统一池化，避免重复分配。
3. **按层管理**：Layer 作为核心查询与渲染分桶维度。
4. **Undo/Redo 走 delta 或轻拷贝**：避免事务走重序列化。
5. **Cap'n Proto 用于持久化**：运行期与事务层不依赖序列化。

### 3.2 关键分歧点
1. **Undo 记录粒度**：
   - 方案 A：对象整拷贝（实现简单，内存风险可控于轻对象）。
   - 方案 B：字段级 delta + 快照指针（复杂但更省内存）。
2. **表达式引擎**：
   - 自研解析器：可控、轻依赖；
   - ExprTk：功能全、落地快、维护成本低。
3. **容器组织**：
   - 仅全局 `ReuseVector<T>`；
   - 全局权威 + per-layer `vector<ObjectId>`（推荐，兼顾跨层引用与层内顺序访问）。

---

## 4. 最终综合架构（本仓库建议收敛版）

### 4.1 内核原则
- **全局权威存储**：所有对象在 type-specific `ReuseVector<T>` 中有稳定 ID。
- **层内热路径索引**：`unordered_map<LayerId, vector<Id>>` 维护 layer -> trace/pin/via 热对象列表。
- **跨层对象只全局存储**：例如 `PadstackDef/BondWire/LayerStack`。
- **字符串最小化策略**：
  - 结构体仅存 `StringId`；
  - 名称搜索通过 StringPool 前缀检索 + 二级倒排（可后续补）。
- **Undo/Redo 事务**：
  - 基础操作为 `add/remove/replace`；
  - 默认对象快照 + 容器 replace；
  - 后续可升级字段级 delta。

### 4.2 对象覆盖（首批）
- 已覆盖：`Layer/Net/PadstackDef/Pad/Drill/Component/Pin/Via/Trace/BondWire`。
- 可扩展：`Board/LayerStack/Port/Surface/Symbol/Region/Text/Constraint/ThermalRelief/DiffPair`。

### 4.3 参数表与变量
- `ParamTable` 使用变量池（`StringId -> value`），当前实现轻量支持 `a+b` / `a*b`。
- 若进入生产版本，建议对接 ExprTk 并做编译缓存。

### 4.4 序列化策略
- 规范层使用 `.capnp` schema（建议继续沿用多版本文档中已有字段模型）。
- 运行内核不依赖序列化；保存/加载阶段做映射转换。

---

## 5. 本次落地代码说明（可编译）

已新增目录 `car_final/`：
- `include/cae_core.hpp`：轻量核心数据结构、StringPool、ReuseVector、UndoRedo、ParamTable、BoardDb。
- `src/main.cpp`：最小演示。
- `tests/test_core.cpp`：核心行为回归测试。
- `CMakeLists.txt`：可直接构建并运行测试。

这份代码是“综合方案的可运行最小内核”，用于确认架构可执行性，并作为后续继续融合 Cap'n Proto/2D/3D/约束求解器 的主干。

---

## 6. 待办列表（下一轮建议）

1. 引入 `car_pcb.capnp` 并完成 `save/load` 映射函数（按当前实体表）。
2. Undo 从对象级快照升级到字段级 delta（Trace 大对象优先）。
3. StringPool 增加倒排索引（name/description 全文匹配）。
4. LayerIndex 增加空间索引（RTree/QuadTree）并与 layer filter 联动。
5. 补齐实体：`Board/LayerStack/Port/Surface/Symbol/Text/Constraint`。
6. 新增事务批处理 API：`begin_tx/commit/rollback` + 批量 replace。
7. 2D/3D View 与内核解耦接口：SceneAdapter + RenderCache。
