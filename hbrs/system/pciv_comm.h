#pragma once

#include <vector>
#include <map>

#include <json/json.h>

#include "common/global.h"

namespace rs
{
namespace pciv
{

struct Msg
{
    enum Type
    {
        READ_DONE = 0,
        WRITE_DONE
    };

    static const int MaxDataLen = 128;

    int type;
    uint8_t data[MaxDataLen];

} __attribute__((aligned(1)));

struct MemoryInfo
{
    uint32_t phy_addr;
    uint32_t size;
} __attribute__((aligned(1)));

struct PosInfo
{
    uint32_t start_pos;
    uint32_t end_pos;
} __attribute__((aligned(1)));

struct StreamInfo
{
    uint32_t align_len;
    uint32_t len;
    uint64_t pts;
    int vdec_chn;
} __attribute__((aligned(1)));

class Context
{
public:
    virtual ~Context() {}

    virtual const std::vector<int> &GetRemoteIds() = 0;

    virtual int Send(int remote_id, int port, uint8_t *data, int len) = 0;

    virtual int Recv(int remote_id, int port, uint8_t *data, int len) = 0;

    virtual int GetTransReadPort() = 0;

    virtual int GetTransWritePort() = 0;

    virtual int GetCMDPort() = 0;
};
} // namespace pciv

class PCIVComm : public pciv::Context
{
public:
    virtual ~PCIVComm();

    static PCIVComm *Instance();

    int Initialize();

    void Close();

    const std::vector<int> &GetRemoteIds() override;

    virtual int Send(int remote_id, int port, uint8_t *data, int len) override;

    virtual int Recv(int remote_id, int port, uint8_t *data, int len) override;

    int GetTransReadPort() override;

    int GetTransWritePort() override;

    int GetCMDPort() override;

protected:
    static int EnumChip(int &local_id, std::vector<int> &remote_ids);

    static int OpenPort(int remote_id, int port, std::vector<std::vector<int>> &remote_fds);

    static int WaitConn(int remote_id);

    explicit PCIVComm();

private:
    int local_id_;
    std::vector<std::vector<int>> remote_fds_;
    std::vector<int> remote_ids_;
    bool init_;

    static const int CommCMDPort;
    static const int TransReadPort;
    static const int TransWritePort;
    static const int MsgPortBase;
    static const int MaxPortNum;
};
}; // namespace rs