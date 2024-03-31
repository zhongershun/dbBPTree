#ifndef OPTIONS_H
#define OPTIONS_H

#include <cstddef>
#include <cstdint>
#include "util/comp.h"
#include "config.h"

class Options
{
public:
    Options(){
        inner_node_children_number = 2;
        leaf_node_record_count = 2;
        inner_node_msg_number = 2;
        comparator = new DefaultCompare<IndexKey>;
    }
    Options(int order){
        inner_node_children_number = order;
        leaf_node_record_count = order;
        inner_node_msg_number = order;
        comparator = new DefaultCompare<IndexKey>;
    };
    ~Options(){};

    DefaultCompare<IndexKey>*comparator;


    // Maximum children number of iner node
    size_t inner_node_children_number;
    size_t inner_node_msg_number;
    
    // Maxium count of records in LeafNode
    // For writing testcase
    size_t leaf_node_record_count;
};

#endif
