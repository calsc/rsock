//
// Created by System Administrator on 1/17/18.
//

#include <cassert>
#include <plog/Log.h>
#include "ServerGroup.h"
#include "../bean/ConnInfo.h"
#include "../util/rhash.h"
#include "../util/rsutil.h"
#include "SNetGroup.h"
#include "../net/INetManager.h"
#include "SubGroup.h"

using namespace std::placeholders;

ServerGroup::ServerGroup(const std::string &groupId, uv_loop_t *loop, const struct sockaddr *target, IConn *btm,
                         INetManager *netManager) : IGroup(groupId, btm) {
    mLoop = loop;
    mTarget = new_addr(target);
    mNetManager = netManager;
}

void ServerGroup::Close() {
    IGroup::Close();
    if (mTarget) {
        free(mTarget);
        mTarget = nullptr;
    }
}

int ServerGroup::OnRecv(ssize_t nread, const rbuf_t &rbuf) {
    if (nread > 0) {
        ConnInfo *info = static_cast<ConnInfo *>(rbuf.data);
        assert(info);
        EncHead *hd = info->head;
        assert(hd);
        auto groupId = IdBuf2Str(hd->IdBuf());
        auto conn = ConnOfKey(groupId);
        if (!conn) {
            conn = newConn(groupId, mLoop, mTarget, *info);
        }
        if (conn) {
            return conn->Input(nread, rbuf);
        }
        LOGE << "no such conn";
        return -1;
    }
    return nread;

}

IConn *ServerGroup::newConn(const std::string &groupId, uv_loop_t *loop, const struct sockaddr *target,
                            const ConnInfo &info) {
    auto fakenet = new SNetGroup(groupId, loop, mNetManager);
    auto conn = new SubGroup(groupId, loop, target, fakenet, nullptr, GetDstAddrStr(info));
    LOGD << "new group: " << GetDstAddrStr(info) << ", groupId: " << groupId;
    if (conn->Init()) {
        conn->Close();
        delete conn;
        return nullptr;
    }
    auto out = std::bind(&IConn::Send, this, _1, _2);
    AddConn(conn, out, nullptr);
    return conn;
}

bool ServerGroup::OnTcpFinOrRst(const TcpInfo &info) {
    auto &conns = GetAllConns();
    for (auto &e: conns) {
        // if cast ot observer, there may crash. reinterpret_cast problem of multiple inheritance
//        auto *observer = reinterpret_cast<ITcpObserver *>(e.second);
        auto *observer = dynamic_cast<SubGroup *>(e.second);

        assert(observer);
        if (observer->OnTcpFinOrRst(info)) {
            if (!e.second->Alive()) {
                CloseConn(e.second);    // close and remove group if dead
            }
            return true;
        }
    }
    return false;
}

const std::string ServerGroup::ToStr() {
    return "ServerGroup";
}
