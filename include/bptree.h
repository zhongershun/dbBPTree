// 这一部分可以充当封装使得外部的都可以直接使用所有头文件

// BPTree
#include "BPTree/node.h"
#include "BPTree/nodePool.h"
#include "BPTree/tree.h"

// db
#include "db/db_impl.h"

// storage
#include "storage/List.h"
#include "storage/Record.h"
#include "storage/tuple.h"

// system
#include "system/config.h"
#include "system/global.h"
#include "system/options.h"
#include "system/ThreadPool.h"

// util
#include "util/comp.h"
#include "util/db_latch.h"
#include "util/db_rw_lock.h"