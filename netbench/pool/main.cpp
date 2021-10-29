#include <iostream>
#include <math.h>
#include <ctime>
#include <time.h>
#include <string>
#include <sstream>
#include <cstring>
#include <array>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <dirent.h>

#include<pthread.h>

#include <openssl/evp.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include "src/pir.hpp"
#include "src/adprp.hpp"
#include "src/client.hpp"
#include "src/server.hpp"

#include <boost/math/distributions/hypergeometric.hpp>
#include <random>

using namespace std;


typedef struct {
    int client_no;
    OfflineAddQueryShort qry;
    //OfflineQuery qry;
    //OnlineQuery qry;
} testQuery;

typedef struct {
    int client_no;
    OfflineReply reply;
    //OnlineReply reply;
} testReply;

vector<testQuery> query_pool;
vector<testReply> reply_pool;


int counter = 0;
int task_done = 0;

vector<testQuery> backup_pool;

#define T 1000  // task pool size

vector<time_t> time_stamp_put(T);
vector<time_t> time_stamp_get(T);


int backup_size = 100;

std::time_t getTimeStamp()
{
    std::chrono::time_point<std::chrono::system_clock,std::chrono::microseconds> tp =
            std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());//获取当前时间点
    std::time_t timestamp =  tp.time_since_epoch().count(); //计算距离1970-1-1,00:00的时间长度
    return timestamp;
}

void myclient_inc() {

    testQuery test_query;

    while (1) {

        // generate randomized request in other pool,
        // and in this function, just fetch from that pool
        // and put it into request_pool

        if (!backup_pool.empty()) {
            test_query = *(backup_pool.begin());
            backup_pool.erase(backup_pool.begin());
            query_pool.push_back(test_query);

            counter++;

            usleep(10000); // every 1 sec put in a query (offer load)
        }

    }
}

void myclient_online() {

    testQuery test_query;

    while (1) {

        if (!backup_pool.empty()) {
            test_query = *(backup_pool.begin());
            backup_pool.erase(backup_pool.begin());

            int task_no = test_query.client_no;
            query_pool.push_back(test_query);

            time_t timestamp = getTimeStamp();
            time_stamp_put[task_no] = timestamp;

            counter++;

            usleep(1000); // every 1 sec put in a query (offer load)
        }
    }
}

void testfunc1() {
    printf("test func 1\n");
}
void testfunc2() {
    printf("test func 2\n");
}

long mean_rep_time = 0;

void server_process_scratch() {
    PIRServer server;
    uint32_t db_size = 7070;

    std::random_device rd;
    Database my_database(db_size);
    for (int i = 0; i < db_size; i++) {
        auto val = generateRandBlock();
        my_database[i] = val;
    }

    server.set_database(db_size, &my_database);

    testQuery query;
    testReply reply;
    OfflineQuery offline_query;
    OfflineReply offline_reply;

    auto server_start = chrono::high_resolution_clock::now();


}

/*
void server_process_online() {
    PIRServer server;
    uint32_t db_size = 7000;

    std::random_device rd;
    Database my_database(db_size);
    for (int i = 0; i < db_size; i++) {
        auto val = generateRandBlock();
        my_database[i] = val;
    }

    server.set_database(db_size, &my_database);

    OnlineQuery online_qry;
    OnlineReply online_reply;

    cout << "server start processing" << endl;

    auto server_st = chrono::high_resolution_clock::now();

    while(1) {

        if (task_done < counter) {

            int task_no = query_pool[task_done].client_no;
            online_qry = query_pool[task_done].qry;

            auto rep_st = chrono::high_resolution_clock::now();

            online_reply = server.generate_online_reply(online_qry, 1);

            time_t timestamp = getTimeStamp();
            time_stamp_get[task_no] = timestamp;

            auto rep_ed = chrono::high_resolution_clock::now();

            //reply_pool.push_back(reply);

            auto rep_time = chrono::duration_cast<chrono::microseconds>
                    (rep_ed - rep_st).count();
            mean_rep_time += rep_time;

            task_done++;

        }

        auto server_ed = chrono::high_resolution_clock::now();
        auto server_time = chrono::duration_cast<chrono::microseconds>
                (server_ed-server_st).count();

        if (server_time >= 100000) {
            cout << "client put in vector in total = " << counter << endl;
            cout << "#task done by server=" << task_done << endl;
            cout << "mean_rep_time = " << (double(mean_rep_time)/double(task_done)) << endl;

            int sumtime = 0;

            for (int tno = 0; tno < task_done; tno++) {
                long diff = time_stamp_get[tno] - time_stamp_put[tno];
                //cout << time_stamp_get[tno] << endl;
                //cout << time_stamp_put[tno] << endl;
                sumtime += diff;
            }
            cout << "per task = " << double(sumtime)/double(task_done) << "msec" << endl;


            exit(0);
        }
    }
}
 */

void server_process_inc() {

    PIRServer server;
    uint32_t db_size = 7000;

    std::random_device rd;
    Database my_database(db_size);
    for (int i = 0; i < db_size; i++) {
        auto val = generateRandBlock();
        my_database[i] = val;
    }

    server.set_database(db_size, &my_database);

    int nbr_add = 0.01 * db_size;

    std::vector<Block> v(nbr_add);
    for (int i = 0; i < nbr_add; i++) {
        v[i] = rand()%100000;
    }


    server.add_elements(nbr_add, v);

    testQuery query;
    testReply reply;
    OfflineAddQueryShort offline_add_qry;
    //OfflineQuery offline_qry;
    OfflineReply offline_reply;


    auto server_st = chrono::high_resolution_clock::now();

    while(1) {

        if (task_done < counter) {

            auto rep_st = chrono::high_resolution_clock::now();

            query = query_pool[task_done];
            int cno = query.client_no;
            offline_add_qry = query.qry;

            offline_reply = server.batched_addition_reply(offline_add_qry);

            time_t timestamp = getTimeStamp();
            time_stamp_get[cno] = timestamp;

            //offline_reply = server.generate_offline_reply(offline_qry,1);

            //for (int i = 0; i < 10000000; i++) {
            //    int k; k++; k = k/199999;
            //}

            auto rep_ed = chrono::high_resolution_clock::now();

            reply.client_no = cno;
            reply.reply = offline_reply;
            //reply_pool.push_back(reply);

            auto rep_time = chrono::duration_cast<chrono::microseconds>
                    (rep_ed-rep_st).count();
            mean_rep_time += rep_time;

            task_done++;

        }

        auto server_ed = chrono::high_resolution_clock::now();
        auto server_time = chrono::duration_cast<chrono::seconds>
                (server_ed-server_st).count();
        if (server_time >= 10) {
            cout << "client put in vector in total = " << counter << endl;
            cout << "#task done by server=" << task_done << endl;
            cout << "mean_rep_time = " << long(double(mean_rep_time)/double(task_done)) << endl;

            int sumtime = 0;

            for (int tno = 0; tno < task_done; tno++) {
                long diff = time_stamp_get[tno] - time_stamp_put[tno];

                sumtime += diff;
            }
            cout << "per task = " << double(sumtime)/double(task_done) << "msec" << endl;

            exit(0);
        }
    }
}


int small_func(int q) {
    vector<int> s;
    for (int i = 0; i < 10000; i++) {
        q = (q/3)*(q/2)+i;
        s.push_back(q);
    }
    return q;
}


typedef struct {
    int no;
    int q;
} small_query;

vector<small_query> small_query_pool;
vector<int> small_backup_pool(100000);

vector<time_t> small_tp_put(100000);
vector<time_t> small_tp_get(100000);

void small_server() {

    long mean_time = 0;

    auto svr_st = chrono::high_resolution_clock::now();

    while(1) {
        if (small_query_pool.size()!=0) {
            small_query qry = *(small_query_pool.begin());

            small_query_pool.erase(small_query_pool.begin());

            int no = qry.no;
            int q = qry.q;

            auto proc_st = chrono::high_resolution_clock::now();

            int q2 = small_func(q);

            time_t tp = getTimeStamp();
            small_tp_get[no] = tp;

            auto proc_ed = chrono::high_resolution_clock::now();
            auto proc_time = chrono::duration_cast<chrono::microseconds>
                    (proc_ed-proc_st).count();
            mean_time += proc_time;

            task_done++;
        }

        auto svr_ed = chrono::high_resolution_clock::now();
        auto svr_time = chrono::duration_cast<chrono::microseconds>
                (svr_ed-svr_st).count();
        if (svr_time > 100000) {
            cout << "client put req in pool total = " << counter << endl;
            cout << "small server tasks done = " << task_done << endl;
            cout << "per task time = " << mean_time / task_done << " microsec" << endl;

            long sumtime = 0;
            for (int c = 0; c < task_done; c++) {
                long diff = small_tp_get[c] - small_tp_put[c];
                //cout << "diff = " << diff << endl;
                sumtime += diff;
            }
            cout << "how long it took each individual request to complete:" << sumtime/task_done << endl;

            exit(0);
        }
    }

}

void small_client() {

    while(1) {
        if (counter < small_backup_pool.size()) {

            if (small_query_pool.size() > 2000) sleep(1);

            small_query qry;
            qry.no = counter;
            qry.q = small_backup_pool[counter];

            small_query_pool.push_back(qry);

            time_t tp = getTimeStamp();
            small_tp_put[counter] = tp;

            counter++;
            usleep(600);
        }
    }
}

int main() {


    // benchmark small_func in main
    auto func_st = chrono::high_resolution_clock::now();
    small_func(3);
    auto func_ed = chrono::high_resolution_clock::now();
    auto func_time = chrono::duration_cast<chrono::microseconds>
            (func_ed-func_st).count();

    cout << "small_func benchmarked in main: time = " << func_time << " microsec" << endl;


    for (int i = 0; i < small_backup_pool.size(); i++) {
        small_backup_pool[i] = rand()%100;
    }

    thread threadObj(small_client);
    small_server();
    threadObj.join();


    /*
    srand(2);

    int db_size = 7000;
    uint32_t set_size = 80;
    uint32_t nbr_sets = 100;//90*12;

    int nbr_add = 0.1*db_size;

    for (int i = 0; i < backup_size; i++) {

        PIRClient client;
        client.set_parms(db_size, set_size, nbr_sets);
        client.generate_setkeys();

        OfflineAddQueryShort offline_add_qry = client.batched_addition_query(db_size, nbr_add);

        //OfflineQuery offline_qry = client.generate_offline_query();

        testQuery test_qry;
        test_qry.client_no = i;
        test_qry.qry = offline_add_qry;
        //test_qry.qry = offline_qry;

        backup_pool.push_back(test_qry);
    }
     */


    /*
    PIRClient client;
    client.set_parms(db_size, set_size, nbr_sets);

    for (int i = 0; i < backup_size; i++) {
        client.generate_setkeys();
        OnlineQuery online_qry = client.generate_online_query(rand()%db_size);

        testQuery test_qry;
        test_qry.client_no = i;
        test_qry.qry = online_qry;

        backup_pool.push_back(test_qry);
        //if (i%10 ==0) cout << i << " ";
    }
     */

    //cout << "backup_pool size=" << backup_size<< " built" << endl;


    //thread threadObj2(myclient_inc);
    //server_process_inc();
    //server_process_online();
    //threadObj2.join();



    /* Make test database and set related parms */

    /*
     * Database 7000
     * set size = 80
     * nbr sets = 90*12
     *
     * Database 1000000
     * set size = 1000
     * nbr sets = 1000*12
     * */

    return 0;
}
