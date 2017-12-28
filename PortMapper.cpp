//
// Created on 12/17/17.
//

#include <cstdlib>

#include <algorithm>
#include <syslog.h>
#include "thirdparty/debug.h"
#include "rscomm.h"

#include "PortMapper.h"


void PortMapper::AddDstPort(IUINT16 port) {
    addFn(mDstPorts, port);
}

void PortMapper::RemoveDstPort(IUINT16 port) {
    removeFn(mDstPorts, port);
}

IUINT16 PortMapper::NextDstPort() {
    return nextFn(mDstPorts);
}

IUINT16 PortMapper::NextSrcPort() {
    return nextFn(mSrcPorts);
}

void PortMapper::SetSrcPorts(const PortLists &ports) {
    mSrcPorts = ports;
}

bool PortMapper::addFn(PortLists &vec, IUINT16 port) {
    for (auto e: vec) {
        if (e == port) {
            debug(LOG_ERR, "port %d already in container");
            return false;
        }
    }
    vec.push_back(port);
    return true;
}

bool PortMapper::removeFn(PortLists &vec, IUINT16 port) {
    auto it = std::find(std::begin(vec), std::end(vec), port);
    if (it != std::end(vec)) {
        vec.erase(it);
        return true;
    }

    debug(LOG_ERR, "port %d not in container");
    return false;
}

IUINT16 PortMapper::nextFn(const PortLists &vec) {
    // todo: add srand in main
    int now = rand();
    if (!vec.empty()) {
        int p = vec[now % vec.size()];
        while (p < 20) {
            p = vec[rand() % vec.size()];
        }
        return p;

    } else {
        // if not specified. use random port
        auto p = static_cast<IUINT16>(now % 65536);
        if (p < 20) {
            p = OM_DEF_PORT;
        }
        debug(LOG_ERR, "port lists empty. choose random port: %d", p);
        return p;
    }
}

void PortMapper::SetDstPorts(const PortLists &ports) {
    mDstPorts = ports;
}

PortMapper::PortLists &PortMapper::GetSrcPortLists() {
    return mSrcPorts;
}

PortMapper::PortLists &PortMapper::GetDstPortLists() {
    return mDstPorts;
}
