# FN-DB V2 语法设计

## 1. 项目目标

FN-DB 在 MySQL 客户端与 MySQL Server 之间增加一层数据库中间件，用于管理数据库级函数依赖，并在创建、修改关系模式时执行 2NF、3NF、BCNF 范式检查。

FN-DB 将数据库模式划分为两层：

- **逻辑属性层**：由 FDSET 定义，例如 A、B、C、D。
- **物理字段层**：真实 MySQL 表中的字段，通过映射关联到逻辑属性。

**核心模型：**

```
Database
 │
 ├── FDSET
 │     ├── 属性全集 U
 │     └── 函数依赖集合 F
 │
 ├── Active FD_SET
 ├── NF_MODE
 │
 └── Tables
       │
       ├── Physical Column
       │       ↓ MAP
       └── Logical Attribute
                ↓
            FD Projection
                ↓
            Candidate Keys
                ↓
            2NF / 3NF / BCNF
```

## 2. 基本概念

一个函数依赖集定义为：

> FDSET = (U, F)

其中：

- **U**：逻辑属性全集。
- **F**：用户声明的业务函数依赖集合。

**统一记号规则：**

- 普通属性集合使用方括号表示，例如 [A B C]。
- 函数依赖左右两侧继续使用方括号，例如 [A] -> [B C]。
- 键定义中，单字段主键使用 A INT PRIMARY KEY；复合主键使用 PRIMARY KEY (A, B)。

**例如：**

```
U = {A, B, C, D, E, F, G}
F = {
    B -> C D,
    D -> G
}
```

逻辑属性与数据库中的物理字段可以不同名。例如：

```
逻辑属性 A ←→ X.COL_A
逻辑属性 B ←→ X.COL_B
逻辑属性 A ←→ Y.USER_A
```

因此，不同表中的不同字段可以映射到同一个逻辑属性。

## 3. FDSET 创建

### 3.1 创建空函数依赖集

```sql
CREATE FDSET xxx;
```

### 3.2 创建完整函数依赖集

```sql
CREATE FDSET xxx (
    ATTRIBUTES [A B C D E F G],
    DEFINE f1 [B] -> [C D],
    DEFINE f2 [D] -> [G]
);
```

其中：

- **ATTRIBUTES**：定义逻辑属性全集 U
- **DEFINE**：定义一条函数依赖
- **f1 / f2**：函数依赖名称
- **[B]**：决定因素
- **[C D]**：被决定属性

### 3.3 匿名函数依赖

函数依赖名称可以省略：

```sql
CREATE FDSET xxx (
    ATTRIBUTES [A B C D E],
    DEFINE [A] -> [B C],
    DEFINE [C] -> [D]
);
```

系统自动生成名称：

```
f1
f2
```

自动生成的 FD 名称必须在当前 FD_SET 中唯一。

## 4. FDSET 删除与清空

### 4.1 删除函数依赖集

删除整个 FD_SET，包括属性全集、函数依赖和相关元数据。

```sql
DROP FDSET xxx;
```

如果该 FD_SET 当前处于激活状态，应先执行：

```sql
DEACTIVATE FDSET;
```

### 4.2 清空函数依赖

V2 规定：TRUNCATE FDSET **清空函数依赖及其ATTRIBUTES**。

```sql
TRUNCATE FDSET xxx;
```

## 5. FDSET 激活

### 5.1 激活函数依赖集

V2规定：

```sql
ACTIVATE FDSET xxx;
```

一个数据库同一时间最多激活一个 FDSET。激活之后，FN-DB 使用该函数依赖集对后续关系模式进行检查。

### 5.2 取消激活

```sql
DEACTIVATE FDSET;
```

## 6. FDSET 查询

### 6.1 查看指定 FDSET

```sql
SHOW FDSET xxx;
```

示例输出：

```
FDSET: xxx
ATTRIBUTES:
A B C D E F G
FUNCTIONAL DEPENDENCIES:
f1 : B -> C D
f2 : D -> G
```

### 6.2 查看所有 FDSET

```sql
SHOW FDSET LIST;
```

示例：

```
NAME        STATUS
xxx         ACTIVE
yyy         INACTIVE
zzz         INACTIVE
```

### 6.3 查看当前激活的 FDSET

```sql
SHOW FDSET ACTIVATED;
```

## 7. 修改逻辑属性

### 7.1 增加属性

```sql
ALTER FDSET xxx
ADD ATTRIBUTE H;
```

### 7.2 删除属性

```sql
ALTER FDSET xxx
DROP ATTRIBUTE H;
```

如果属性 H 当前出现在函数依赖中，或被某个物理字段映射，则不允许直接删除。

### 7.3 修改属性名称

```sql
ALTER FDSET xxx
RENAME ATTRIBUTE H TO I;
```

修改后，需要同步更新函数依赖中的属性名与物理字段映射关系。

## 8. 修改函数依赖

### 8.1 添加命名函数依赖

```sql
ALTER FDSET xxx
ADD FD f3 [A] -> [E];
```

### 8.2 添加匿名函数依赖

```sql
ALTER FDSET xxx
ADD FD [A] -> [E];
```

### 8.3 删除函数依赖

```sql
ALTER FDSET xxx
DROP FD f3;
```

### 8.4 修改函数依赖名称

```sql
ALTER FDSET xxx
RENAME FD f2 TO dependency_d_g;
```

### 8.5 修改函数依赖内容

```sql
ALTER FDSET xxx
MODIFY FD f1 TO [B] -> [C];
```

## 9. 逻辑属性与物理字段映射

FN-DB 中 A、B、C、D ... 属于逻辑属性；MySQL 表中的列属于物理字段。

### 9.1 CREATE TABLE 时映射

```sql
CREATE TABLE X (
    COL_A INT AUTO_INCREMENT PRIMARY KEY MAP TO A,
    COL_B INT MAP TO B,
    COL_C VARCHAR(10) MAP TO C,
    COL_D BOOLEAN MAP TO D,
    COL_G INT MAP TO G
);
```

FN-DB 内部记录：

```
X.COL_A -> A
X.COL_B -> B
X.COL_C -> C
X.COL_D -> D
X.COL_G -> G
```

### 9.2 同名属性自动映射

如果物理字段名称与逻辑属性名称一致，可以省略 MAPS TO：

```sql
CREATE TABLE X (
    A INT PRIMARY KEY,
    B INT,
    C VARCHAR(10),
    D BOOLEAN
);
```

如果当前激活的 FD_SET 中存在 A、B、C、D，则自动建立：

```
X.A -> A
X.B -> B
X.C -> C
X.D -> D
```

### 9.3 多张表映射到同一个逻辑属性

```sql
CREATE TABLE X (
    COL_A INT PRIMARY KEY MAPS TO A
);
CREATE TABLE Y (
    USER_A INT MAPS TO A
);
```

此时：

```
X.COL_A -> A
Y.USER_A -> A
```

逻辑上二者都表示属性 A。

## 10. 已存在表的属性映射

### 10.1 添加映射

```sql
ALTER MAPSET X ADD MAP COL_A TO A;
```

### 10.2 解除映射

```sql
ALTER MAPSET X DROP MAP COL_A;
```

### 10.3 修改映射

```sql
ALTER MAPSET X REMAP MAP COL_A TO B;
```

### 10.4 查看表映射

```sql
SHOW MAPPSET X;
```

示例：

```
PHYSICAL COLUMN     LOGICAL ATTRIBUTE
COL_A               A
COL_B               B
COL_C               C
COL_D               D
```

## 11. 范式模式

FDSET 激活状态与范式等级分开管理。

### 11.1 开启 2NF

```sql
SET NF_MODE = 2NF;
```

### 11.2 开启 3NF

```sql
SET NF_MODE = 3NF;
```

### 11.3 开启 BCNF

```sql
SET NF_MODE = BCNF;
```

### 11.4 关闭范式约束

```sql
SET NF_MODE = OFF;
```

### 11.5 查看当前模式

```sql
SHOW NF_MODE;
```

## 12. CREATE TABLE 自动产生的函数依赖

FN-DB 除了读取 FD_SET 中用户声明的业务函数依赖，还可以从表结构中自动得到部分函数依赖。

### 12.1 PRIMARY KEY

单字段主键保持原来的行内写法：

```sql
A INT PRIMARY KEY
```

只有复合主键使用圆括号：

```sql
PRIMARY KEY (A, B)
```

系统自动知道：

```sql
CREATE TABLE X (
    A INT,
    B INT,
    C INT,
    D INT,
    PRIMARY KEY (A, B)
);
```

```
[A B] -> [A B C D]
```

因为主键能够唯一确定一行。

### 12.2 UNIQUE + NOT NULL

系统可以识别：

```sql
CREATE TABLE X (
    A INT PRIMARY KEY,
    B INT UNIQUE NOT NULL,
    C INT,
    D INT
);
```

此时 A 和 B 都属于候选键。

```
[A] -> [A B C D]
[B] -> [A B C D]
```

### 12.3 FOREIGN KEY

外键不直接产生普通函数依赖。

```sql
FOREIGN KEY (B) REFERENCES Y(B)
```

表达的是引用关系，而不是：

```
B -> ...
```

因此 V1 不把外键直接加入 FD 集合。

### 12.4 GENERATED COLUMN

生成列理论上可以产生函数依赖：

```sql
CREATE TABLE X (
    A INT,
    B INT,
    C INT GENERATED ALWAYS AS (A + B)
);
```

可以得到：

```
[A B] -> [C]
```

V1 可以暂不支持自动分析复杂生成表达式，将其作为后续扩展。

## 13. 范式检查

### 13.1 检查单张表

使用当前激活的 FD_SET 和当前 NF_MODE 进行检查。

```sql
CHECK TABLE X;
```

### 13.2 指定范式

```sql
CHECK TABLE X FOR 2NF;
CHECK TABLE X FOR 3NF;
CHECK TABLE X FOR BCNF;
```

### 13.3 检查整个数据库

```sql
CHECK DATABASE;
```

或者：

```sql
CHECK DATABASE FOR 3NF;
```

FN-DB 对数据库中的各张表分别执行范式检查。

## 14. 范式检查流程

假设：

```sql
CREATE FDSET xxx (
    ATTRIBUTES [A B C D E F G],
    DEFINE f1 [B] -> [C D],
    DEFINE f2 [D] -> [G]
);
ACTIVATE FDSET xxx;
SET NF_MODE = 3NF;
```

创建表：

```sql
CREATE TABLE X (
    COL_A INT PRIMARY KEY MAP TO A,
    COL_B INT MAP TO B,
    COL_C VARCHAR(10) MAP TO C,
    COL_D BOOLEAN MAP TO D,
    COL_G INT MAP TO G
);
```

物理表：

```
X(COL_A, COL_B, COL_C, COL_D, COL_G)
```

转换为逻辑关系：

```
X(A, B, C, D, G)
```

FD_SET 中相关依赖：

```
[B] -> [C D]
[D] -> [G]
```

主键产生：

```
[A] -> [A B C D G]
```

随后：

```
R = {A, B, C, D, G}
F = {
    A -> B C D G,
    B -> C D,
    D -> G
}
```

```
计算属性闭包
    ↓
求候选键
    ↓
确定主属性
    ↓
确定非主属性
    ↓
检查部分依赖
    ↓
检查传递依赖
    ↓
检查决定因素是否为超键
    ↓
判断 2NF / 3NF / BCNF
```

## 15. 范式检查结果

例如：

```sql
CHECK TABLE X FOR 3NF;
```

可以返回：

```
TABLE: X
NORMAL FORM: 3NF
RESULT: FAILED
CANDIDATE KEYS:
(A)
VIOLATION:
[D] -> [G]
REASON:
D is not a super key and G is a non-prime attribute.
```

如果通过：

```
TABLE: X
NORMAL FORM: 3NF
RESULT: PASSED
NOTICE:
Result is based on declared functional dependencies.
```

FN-DB 只能根据用户已经声明的函数依赖进行范式判断，无法自动知道所有真实业务语义中的函数依赖。

## 16. 字段合法性检查

当：

```sql
ACTIVATE FDSET xxx;
SET NF_MODE = 3NF;
```

创建新表时，所有参与 FN-DB 范式管理的字段必须能够映射到当前 FD_SET 的逻辑属性全集。

例如：

```
U = {A, B, C, D, E}
```

合法：

```sql
CREATE TABLE X (
    A INT,
    B INT,
    C INT
);
```

因为：

```
{A, B, C} ⊆ U
```

也合法：

```sql
CREATE TABLE X (
    COL_A INT MAP TO A,
    COL_B INT MAP TO B
);
```

非法：

```sql
CREATE TABLE X (
    A INT,
    H INT
);
```

如果：

```
H ∉ U
```

且没有显式映射，则返回错误。

## 17. 推荐关键字

FN-DB V2 自定义关键字：

- FDSET
- ATTRIBUTES
- ATTRIBUTE
- DEFINE
- FD
- ACTIVATE
- DEACTIVATE
- ADD
- DROP
- MODIFY
- RENAME
- MAPSET
- MAP
- CHECK
- FOR
- NF_MODE
- 2NF
- 3NF
- BCNF
- OFF

复用 SQL 原有关键字：

- CREATE
- ALTER
- DROP
- TRUNCATE
- SHOW
- SET
- TABLE
- DATABASE
- TO
- COLUMN
- PRIMARY
- KEY
- UNIQUE
- NOT
- NULL

## 18. 推荐完整示例

逻辑层对应：

```
STUDENT(A, B)
COURSE(C, D, G)
SCORE(A, C, E)
```

数据库级业务函数依赖：

```
A -> B
C -> D
A C -> E
D -> G
```

```sql
CREATE FDSET school (
    ATTRIBUTES [A B C D E F G],
    DEFINE f1 [A] -> [B],
    DEFINE f2 [C] -> [D],
    DEFINE f3 [A C] -> [E],
    DEFINE f4 [D] -> [G]
);
ACTIVATE FDSET school;
SET NF_MODE = 3NF;
CREATE TABLE STUDENT (
    STUDENT_ID INT PRIMARY KEY MAP TO A,
    STUDENT_NAME VARCHAR(32) MAP TO B
);
CREATE TABLE COURSE (
    COURSE_ID INT PRIMARY KEY MAP TO C,
    COURSE_NAME VARCHAR(32) MAP TO D,
    COURSE_EXT VARCHAR(32) MAP TO G
);
CREATE TABLE SCORE (
    STUDENT_ID INT MAP TO A,
    COURSE_ID INT MAP TO C,
    SCORE_VALUE INT MAP TO E,
    PRIMARY KEY (STUDENT_ID, COURSE_ID)
);
CHECK TABLE STUDENT FOR 3NF;
CHECK TABLE COURSE FOR 3NF;
CHECK TABLE SCORE FOR 3NF;
CHECK DATABASE FOR 3NF;
```

表中键约束产生的依赖：

```
STUDENT:
A -> A B
COURSE:
C -> C D G
SCORE:
A C -> A C E
```

系统最终根据：

```
数据库级 FDSET
+
表字段映射
+
CREATE TABLE 中的键约束
```

计算每张表对应的函数依赖，并执行范式检查。

## 19. V2 设计原则

1. 一个数据库可以创建多个 FDSET。
2. 一个数据库同一时间最多激活一个 FDSET。
3. FD_SET 属于数据库级逻辑约束，不直接属于某一张表。
4. ATTRIBUTES 定义数据库级逻辑属性全集。
5. 表中的物理字段通过映射对应逻辑属性。
6. 多张表中的不同物理字段可以映射到同一个逻辑属性。
7. 同名字段默认自动映射。
8. PRIMARY KEY 自动产生键函数依赖。
9. UNIQUE + NOT NULL 可以作为候选键来源。
10. FOREIGN KEY 不直接作为普通函数依赖。
11. 范式检查针对具体关系模式执行。
12. 范式检查只基于当前已知和已声明的函数依赖。
13. 用户未声明的业务函数依赖，FN-DB 不做猜测。
14. NF_MODE 与 FDSET 激活状态相互独立。
15. FN-DB 的目标是检查和解释范式违规，而不是替代用户定义业务语义。
