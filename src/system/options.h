#ifndef OPTIONS_H
#define OPTIONS_H

#include <cstddef>
#include <cstdint>
#include "comp.h"

class Options
{
public:
    Options(){
        inner_node_children_number = 16;
        leaf_node_record_count = 16;
        comparator = new DefaultCompare<IndexKey>;
    };
    ~Options(){};

    DefaultCompare<IndexKey>*comparator;


    // Maximum children number of iner node
    size_t inner_node_children_number;
    
    // Maxium count of records in LeafNode
    // For writing testcase
    size_t leaf_node_record_count;
};

#endif