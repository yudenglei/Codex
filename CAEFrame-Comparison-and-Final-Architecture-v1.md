# CAEFrame 多版本分类对比与最终综合架构（v2）

## 1. 分类与增量识别结论

- **全量替换主线**：ChatGPT v1/v2、Deepseek v2、Grok v10/v14/v15/v16/v17、doubao v1/v2。  
- **增量演进主线**：Grok v11→v12→v13；Gemini v2→v3。  
- **专题扩展（视图层）**：ChatGPT v3、Gemini v4（2D/3D）。

## 2. 综合方案（面向高性能/低内存/可事务回放）

### 2.1 数据层
- 全对象使用稳定 `Id`，容器使用 `ReuseVector<T>`；几何尺寸统一使用 `DbuValue`（literal/param tagged）支持参数化变量。
- 跨层对象（`PadstackDef/BondWire/LayerStack`）全局权威存储。
- 层内热路径对象（如 `Trace`）附加 layer->ids 与 layer spatial index。

### 2.2 字符串层
- `StringPool` 统一池化（`StringId` 引用）。
- 增加倒排索引：分词后 token->StringId[]，支持名称/描述全文检索。

### 2.3 事务层（结合 KLayout 思路）
- 基础动作：`add/remove/replace`，对象轻量化时直接对象快照 + replace。
- 对 `Trace` 这类可能偏大对象：保留后续 delta 升级入口，但**当前不强制字段级 delta**。这是内存与复杂度平衡：在对象已轻量化且 replace 成本可控时，先保持事务链路简单、可验证。
- 新增批处理：`begin_tx/commit_tx/rollback_tx` + `batch_replace`。

### 2.4 参数化层
- 引入表达式引擎抽象接口 `IExprEngine`。
- 默认内置轻量表达式引擎；若检测到 `ExprTk`，自动切换并启用编译缓存。

### 2.5 序列化层
- 引入 `car_pcb.capnp`，按实体表建模。
- `save_capnp/load_capnp` 做映射转换；运行时事务不依赖序列化。

### 2.6 视图解耦层
- 新增 `SceneAdapter` + `RenderCache` 抽象。
- 2D/3D 视图通过 adapter 拉取 primitives，避免 UI 与内核耦合。

## 3. 本次已落地实现

- 实体补齐：`Board/LayerStack/Port/Surface/Symbol/Text/Constraint`。
- 字符串倒排检索。
- Layer 空间索引（QuadTree）+ layer filter 查询。
- 事务批处理 API。
- `car_pcb.capnp` 与 `save/load` 接口。
- SceneAdapter/RenderCache 接口。
- 参数引擎 ExprTk 适配（可选）与缓存框架。

## 4. 下一步（工程化增强）

1. 将 capnp save/load 从“核心实体优先”扩展到全量实体双向映射。
2. 引入版本化 schema + 向前兼容加载策略。
3. Trace 大对象在高压场景下补齐字段级 delta 通道（可选开关）。
4. 将 QuadTree 升级为批量构建 + 并发查询优化版本。
5. 增加 2D/3D 适配层基准与缓存失效策略测试。
