#include "client.hpp"
#include "serial.hpp"
#include "net.hpp"

using namespace chrono;

class NetClient : public PIRClient {

    int get_fd(string ip) {
        int fd = connect_to_addr(ip);
        assert(fd > 0);
        return fd;
    }

public:
    NetClient(uint32_t db_size, uint32_t set_size, uint32_t nbr_sets) {
        set_parms(db_size, set_size, nbr_sets);
        generate_setkeys();
    }

    void offline_query(string ip) {
        OfflineQuery offline_qry = generate_offline_query();
        int fd = get_fd(ip);
        string q_str = serialize_offline_query(offline_qry);
        string query_str = serializeQuery(q_str, interface::QueryType::OFFLINE);
        sendMsg(fd, query_str);
        string response;
        assert(recvMsg(fd, response));
        OfflineReply offline_reply = deserialize_offline_reply(response);
        update_local_hints(offline_reply);
        close(fd);
    }

    Block online_query(string ip, OnlineQuery online_qry, double& total_time) {
        system_clock::time_point start, end;
        start = system_clock::now();
        int fd = get_fd(ip);
        string q_str = serialize_online_query(online_qry);
        string query_str = serializeQuery(q_str, interface::QueryType::ONLINE);
        sendMsg(fd, query_str);
        string response;
        assert(recvMsg(fd, response));
        OnlineReply online_reply = deserialize_online_reply(response);
        Block blk = recover_block(online_reply);
        end = system_clock::now();
        total_time = duration_cast<std::chrono::duration<double>>(end - start).count();
        close(fd);
        return blk;
    }

    void refresh_query(string ip, OnlineQuery online_qry, double& total_time, Block& blk) {
        system_clock::time_point start, end;
        start = system_clock::now();
        int fd = get_fd(ip);
        string q_str = serialize_online_query(online_qry);
        string query_str = serializeQuery(q_str, interface::QueryType::REFRESH);
        sendMsg(fd, query_str);
        string response;
        assert(recvMsg(fd, response));
        OnlineReply refresh_reply = deserialize_online_reply(response);
        sets[cur_qry_setno].hint = blk ^ refresh_reply.parity;
        end = system_clock::now();
        total_time = duration_cast<std::chrono::duration<double>>(end - start).count();
        close(fd);
        return;
    }

    void offline_query_baseline(string ip, OfflineQuery& offline_qry, double& total_time) {
        system_clock::time_point start, end;
        start = system_clock::now();
        int fd = get_fd(ip);

        system_clock::time_point start_tmp, end_tmp;
        start_tmp = system_clock::now();
        string q_str = serialize_offline_query(offline_qry);
        string query_str = serializeQuery(q_str, interface::QueryType::OFFLINE);
        end_tmp = system_clock::now();
        cout << "client serialize query: " << duration_cast<std::chrono::duration<double>>(end_tmp - start_tmp).count() << endl;
        
        sendMsg(fd, query_str);
        string response;
        assert(recvMsg(fd, response));

        start_tmp = system_clock::now();
        OfflineReply offline_reply = deserialize_offline_reply(response);
        end_tmp = system_clock::now();
        cout << "client deserialize reply: " << duration_cast<std::chrono::duration<double>>(end_tmp - start_tmp).count() << endl;
        
        start_tmp = system_clock::now();
        update_local_hints(offline_reply);
        end_tmp = system_clock::now();
        cout << "client update hints: " << duration_cast<std::chrono::duration<double>>(end_tmp - start_tmp).count() << endl;
        
        end = system_clock::now();
        total_time = duration_cast<std::chrono::duration<double>>(end - start).count();
        close(fd);
    }

};

