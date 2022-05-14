
#include "mapper/configuration.h"

// get config data for GPE, return<LSB-location, CfgData>
std::map<int, CfgData> Configuration::getGpeCfgData(GPENode* node){
    int adgNodeId = node->id();
    ADG* subAdg = node->subADG();
    auto& adgNodeAttr = _mapping->adgNodeAttr(adgNodeId);
    DFGNode* dfgNode = adgNodeAttr.dfgNode;
    if(dfgNode == nullptr){
        return {};
    }
    std::map<int, CfgData> cfg;
    // operation
    int opc = Operations::OPC(dfgNode->operation());
    int aluId = -1;
    // bool flag = false;
    std::set<int> usedOperands;
    for(auto& elem : dfgNode->inputEdges()){
        int eid = elem.second;
        auto& edgeAttr = _mapping->dfgEdgeAttr(eid);
        int delay = edgeAttr.delay; // delay cycles
        int inputIdx = edgeAttr.edgeLinks.rbegin()->srcPort; // last edgeLInk, dst port
        auto muxPair = subAdg->input(inputIdx).begin(); // one input only connected to one Mux
        int muxId = muxPair->first;
        int muxCfgData = muxPair->second;
        auto mux = subAdg->node(muxId);
        int dlypipeId = mux->output(0).begin()->first; 
        auto dlypipe = subAdg->node(dlypipeId);   
        auto aluPair = dlypipe->output(0).begin();   
        aluId = aluPair->first;
        usedOperands.emplace(aluPair->second); // operand index
        CfgDataLoc muxCfgLoc = node->configInfo(muxId);
        CfgData muxCfg((muxCfgLoc.high - muxCfgLoc.low + 1), (uint32_t)muxCfgData);
        cfg[muxCfgLoc.low] = muxCfg;
        CfgDataLoc dlyCfgLoc = node->configInfo(dlypipeId);
        CfgData dlyCfg(dlyCfgLoc.high - dlyCfgLoc.low + 1, (uint32_t)delay);
        cfg[dlyCfgLoc.low] = dlyCfg;        
    }
    if(aluId == -1){ // in case that some node has no input (not completed, should be avoided)
        auto muxPair = subAdg->input(0).begin(); // one input only connected to one Mux
        int muxId = muxPair->first;
        auto mux = subAdg->node(muxId);
        int dlypipeId = mux->output(0).begin()->first; 
        auto dlypipe = subAdg->node(dlypipeId);   
        auto aluPair = dlypipe->output(0).begin();   
        aluId = aluPair->first;
    }
    CfgDataLoc aluCfgLoc = node->configInfo(aluId);
    CfgData aluCfg(aluCfgLoc.high - aluCfgLoc.low + 1, (uint32_t)opc);
    cfg[aluCfgLoc.low] = aluCfg;
    // Constant
    if(dfgNode->hasImm()){
        // find unused operand
        int i = 0;
        for(; i < dfgNode->numInputs(); i++){
            if(!usedOperands.count(i)){ 
                break;
            }
        }
        auto alu = subAdg->node(aluId); 
        auto dlypipe = subAdg->node(alu->input(i).first); // used default delay 
        int muxId = dlypipe->input(0).first;
        for(auto& elem : subAdg->node(muxId)->inputs()){
            int id = elem.second.first;
            if(id == subAdg->id()) continue;
            if(subAdg->node(id)->type() == "Const"){
                CfgDataLoc muxCfgLoc = node->configInfo(muxId);
                CfgData muxCfg(muxCfgLoc.high - muxCfgLoc.low + 1, (uint32_t)elem.first);
                cfg[muxCfgLoc.low] = muxCfg;
                CfgDataLoc constCfgLoc = node->configInfo(id);                
                int len = constCfgLoc.high - constCfgLoc.low + 1;
                CfgData constCfg(len);
                uint64_t imm = dfgNode->imm();
                while(len > 0){
                    constCfg.data.push_back(uint32_t(imm&0xffffffff));
                    len -= 32;
                    imm >> 32;
                }
                cfg[constCfgLoc.low] = constCfg;
                break;
            }
        }  
    }
    return cfg;
}


// get config data for GIB and IOB, return<LSB-location, CfgData>
std::map<int, CfgData> Configuration::getGibIobCfgData(ADGNode* node){
    int adgNodeId = node->id();
    ADG* subAdg = node->subADG();
    auto& adgNodeAttr = _mapping->adgNodeAttr(adgNodeId);
    auto& passEdges = adgNodeAttr.dfgEdgePass;
    if(passEdges.empty()){
        return {};
    }
    std::map<int, CfgData> cfg;
    for(auto& elem : passEdges){
        int muxId = subAdg->output(elem.dstPort).first; // one output connected to one mux
        if(muxId == subAdg->id()){ // actually connected to input port
            continue;
        }
        auto mux = subAdg->node(muxId);        
        // find srcPort
        for(auto in : mux->inputs()){
            if(in.second.second == elem.srcPort){ 
                CfgDataLoc muxCfgLoc = node->configInfo(muxId);
                CfgData muxCfg(muxCfgLoc.high - muxCfgLoc.low + 1, (uint32_t)in.first);
                cfg[muxCfgLoc.low] = muxCfg;
                break;
            }
        }
    }
    return cfg;
}


// get config data for ADG node
void Configuration::getNodeCfgData(ADGNode* node, std::vector<CfgDataPacket>& cfg){
    std::map<int, CfgData> cfgMap;
    if(node->type() == "GPE"){
        cfgMap = getGpeCfgData(dynamic_cast<GPENode*>(node));
    }else if(node->type() == "GIB" || node->type() == "OB"){
        cfgMap = getGibIobCfgData(node);
    }else if(node->type() == "IB" && node->numInputs() > 1){ // only if input number > 1, there is config data
        cfgMap = getGibIobCfgData(node);
    }
    if(cfgMap.empty()){
        return;
    }
    ADG* adg = _mapping->getADG();
    int cfgDataWidth = adg->cfgDataWidth();
    int totalLen = cfgMap.rbegin()->first + cfgMap.rbegin()->second.len;
    int num = (totalLen+31)/32;
    std::vector<uint32_t> cfgDataVec(num, 0);
    std::set<uint32_t> addrs;
    for(auto& elem : cfgMap){ // std::map auto-sort keys
        int lsb = elem.first;
        int len = elem.second.len;
        auto& data = elem.second.data;
        // cache valid address
        uint32_t targetAddr = lsb/cfgDataWidth;
        int addrNum = (len + (lsb%cfgDataWidth) + cfgDataWidth - 1)/cfgDataWidth;
        for(int i = 0; i < addrNum; i++){
            addrs.emplace(targetAddr+i);
        } 
        // cache data from 0 to MSB   
        int targetIdx = lsb/32;
        int offset = lsb%32;
        uint64_t tmpData = data[0];
        int dataIdx = 0;
        int dataLenLeft = 32;
        while(len > 0){
            if(len <= 32 - offset){
                len = 0;
                cfgDataVec[targetIdx] |= (tmpData << offset);
            }else{                          
                dataLenLeft -= 32 - offset; 
                cfgDataVec[targetIdx] |= (tmpData << offset);                
                targetIdx++;
                dataIdx++;
                tmpData >>= 32 - offset;
                if(dataIdx < data.size()){
                    tmpData |= data[dataIdx] << dataLenLeft;
                    dataLenLeft += 32;
                }
                len -= 32 - offset;
                offset = 0;
            }
        }
    }
    // construct CfgDataPacket
    int cfgBlkOffset = adg->cfgBlkOffset();
    int cfgBlkIdx = node->cfgBlkIdx();
    // int x = node->x();
    uint32_t highAddr = uint32_t(cfgBlkIdx << cfgBlkOffset);
    int n;
    int mask;
    if(cfgDataWidth >= 32){
        assert(cfgDataWidth%32 == 0);
        n = cfgDataWidth/32;
    }else{
        assert(32%cfgDataWidth == 0);
        n = 32/cfgDataWidth;
        mask = (1 << cfgDataWidth) - 1;
    }
    for(auto addr : addrs){
        CfgDataPacket cdp(highAddr|addr);
        if(cfgDataWidth >= 32){
            int size = cfgDataVec.size();
            for(int i = 0; i < n; i++){
                int idx = addr*n+i;
                uint32_t data = (idx < size)? cfgDataVec[idx] : 0;
                cdp.data.push_back(data);
            }
        }else{
            uint32_t data = (cfgDataVec[addr/n] >> ((addr%n)*cfgDataWidth)) & mask;
            cdp.data.push_back(data);
        }
        cfg.push_back(cdp);
    }
}


// get config data for ADG
void Configuration::getCfgData(std::vector<CfgDataPacket>& cfg){
    cfg.clear();
    for(auto& elem : _mapping->getADG()->nodes()){
        getNodeCfgData(elem.second, cfg);
    }
}


// dump config data
void Configuration::dumpCfgData(std::ostream& os){
    std::vector<CfgDataPacket> cfg;
    getCfgData(cfg);
    ADG* adg = _mapping->getADG();
    int cfgAddrWidth = adg->cfgAddrWidth();
    int cfgDataWidth = adg->cfgDataWidth();
    int addrWidthHex = (cfgAddrWidth+3)/4;
    int dataWidthHex = std::min(cfgDataWidth/4, 8);
    os << std::hex;
    for(auto& cdp : cfg){
        os << std::setw(addrWidthHex) << std::setfill('0') << (cdp.addr) << " ";
        for(int i = cdp.data.size() - 1; i >= 0; i--){
            os << std::setw(dataWidthHex) << std::setfill('0') << cdp.data[i];
        }
        os << std::endl;
    }
    os << std::dec;
}
