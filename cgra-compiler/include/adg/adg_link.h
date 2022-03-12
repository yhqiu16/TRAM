#ifndef __ADG_LINK_H__
#define __ADG_LINK_H__

#include <iostream>
#include <map>
#include <vector>
#include <set>
// #include "adg/adg_node.h"

class ADGLink
{
private:
    int _id;
    int _srcPortIdx; // source node I/O port index
    int _dstPortIdx; // destination node I/O port index
    int _srcId;   // source node ID
    int _dstId;   // destination node ID
public:
    ADGLink(){}
    ADGLink(int linkId){ _id = linkId; }
    ADGLink(int srcId, int dstId) : _srcId(srcId), _dstId(dstId) {}
    ~ADGLink(){}
    int id(){ return _id; }
    void setId(int id){ _id = id; }
    int srcPortIdx(){ return _srcPortIdx; }
    void setSrcPortIdx(int srcPortIdx){ _srcPortIdx = srcPortIdx; }
    int dstPortIdx(){ return _dstPortIdx; }
    void setDstPortIdx(int dstPortIdx){ _dstPortIdx = dstPortIdx; }
    int srcId(){ return _srcId; }
    void setSrcId(int srcId){ _srcId = srcId; }
    int dstId(){ return _dstId; }
    void setDstId(int dstId){ _dstId = dstId; }
    void setLink(int srcId, int dstId){
        _srcId = srcId;
        _dstId = dstId;
    }
};






#endif