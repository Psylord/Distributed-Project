//
// Created by jai on 18/11/18.
//

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mpi.h>
#include <unistd.h>
#include <mutex>
#include <queue>
#include <vector>
#include <iterator>
#include <random>
#include <stack>
int total = 5;
using namespace std;
void normal( int id , int leader , int size);
void leader_function( int id , int size);
void send(vector<int> msg ,  int id)
{
    MPI_Send(&msg[0], 6 , MPI_INT, id, 0, MPI_COMM_WORLD);
}
void sendall(vector<int> msg , int id, int size)
{
    for (int i = 0; i < size ; ++i) {
        if(id != i)
            send( msg , i );
    }
}
vector<int> receive(int id)
{
    vector<int> msg;
    msg.resize(6);
    MPI_Recv(&msg[0] ,6 ,MPI_INT ,id , 0 , MPI_COMM_WORLD ,  MPI_STATUS_IGNORE);
    return msg;
}
vector<int> receiveany()
{
    vector<int> msg;
    msg.resize(6);
    MPI_Recv(&msg[0] ,6 ,MPI_INT , MPI_ANY_SOURCE , 0 , MPI_COMM_WORLD ,  MPI_STATUS_IGNORE);
    return msg;
}
vector<int> election_result(int id, int amiin)
{
    int n = amiin;
    int flag = 1;
    MPI_Status stat;
    vector<int> result;
    while(flag)
    {
        MPI_Iprobe(MPI_ANY_SOURCE , MPI_ANY_TAG , MPI_COMM_WORLD , &flag , &stat);
        n = n + flag;
        //cout << "flag " << flag << "\n" ;
        if(flag) {
            //cout <<"source " << stat.MPI_SOURCE << "\n";
            vector<int> msg = receive(stat.MPI_SOURCE);
            if(msg[0] != 0)
                n = n - flag;
        }
    }
    result.push_back(n);
    if(n == 1 )
    {
        if(amiin == 0)
            result.push_back(stat.MPI_SOURCE );
        else
            result.push_back(id );
    }
    return  result;
}

void leader_election(int id , int size)
{
    if(total == 0)
        exit(0);
    total--;
    MPI_Barrier(MPI_COMM_WORLD);
    int i = 1;
    int leader;
    int amiin = 0;
    while (true)
    {

        i = 2*i;
        mt19937 rng;
        rng.seed(random_device()());
        uniform_int_distribution<mt19937::result_type> dist(1, static_cast<unsigned long>(pow(2 ,i)));
        unsigned long r = dist(rng);

        if(r == 1) {
            vector<int> msg;
            msg.push_back(0);
            msg.push_back(id);
            sendall(msg , id , size);
            amiin = 1;
        }
        MPI_Barrier(MPI_COMM_WORLD);
        vector<int> result = election_result(id , amiin);
        if(result[0] == 0)
            break;
        amiin = 0;
    }
    MPI_Barrier(MPI_COMM_WORLD);
    int l = static_cast<int>(pow(2 , i/2));
    int u = static_cast<int>(pow(2 , i));
    amiin = 0;
    while( l + 1 < u)
    {
        int j = (l + u)/2;
        mt19937 rng;
        rng.seed(random_device()());
        uniform_int_distribution<mt19937::result_type> dist(1, static_cast<unsigned long>(pow(2 ,j)));
        unsigned long r = dist(rng);

        if(r == 1) {
            vector<int> msg;
            msg.push_back(0);
            msg.push_back(id);
            sendall(msg , id , size);
            amiin = 1;
        }
        MPI_Barrier(MPI_COMM_WORLD);
        vector<int> result = election_result(id , amiin);

        if(result[0] == 0)
            u = j;
        else
            l = j;
        MPI_Barrier(MPI_COMM_WORLD);
        amiin = 0;
    }
    int k = u;
    amiin = 0;
    while (true)
    {


        mt19937 rng;
        rng.seed(random_device()());
        uniform_int_distribution<mt19937::result_type> dist(1, static_cast<unsigned long>(pow(2 ,k)));
        unsigned long r = dist(rng);

        if(r == 1) {
            vector<int> msg;
            msg.push_back(0);
            msg.push_back(id);
            sendall(msg , id , size);
            amiin = 1;
        }
        MPI_Barrier(MPI_COMM_WORLD);
        vector<int> result = election_result(id , amiin);
        if(result[0] == 0)
            k = k - 1;
        else
            k = k + 1;
        if(result[0] == 1)
        {
            leader = result[1];
            break;
        }
        amiin = 0;
        MPI_Barrier(MPI_COMM_WORLD);
    }

    if(id == leader)
        leader_function(id , size);
    else
        normal(id , leader , size);
}
void leader_function(int id , int size)
{
    cout << "Leader is process " << id << "\n";
    mt19937 rng;
    rng.seed(random_device()());
    uniform_int_distribution<mt19937::result_type> dist(1, 40);
    unsigned long r = dist(rng);

    stack <int> s;
    while (r > 0)
    {
        vector<int> msg = receiveany();
        if (msg[0] == 1)
        {
            s.push(msg[1]);
            cout<<"Leader recieves message " << msg[1]<<"\n";
        }
        else if(msg[0] == 2)
        {
            vector<int> reply;
            reply.push_back(3);
            if(s.empty())
                reply.push_back(-1);
            else
                reply.push_back(s.top());
            cout<<"Leader sends message "<<reply[1] << " to " << msg[1] <<"\n";
            send(reply , msg[1]);

        }
        r--;
    }
    vector<int> faulty;
    faulty.push_back(4);
    sendall(faulty , id , size);
    cout<< " Leader bacomes faulty " << "\n";
    leader_election(id , size);
}
void normal( int id , int leader , int size)
{
    cout << "Process "<<id <<" knows Leader is process " << leader << "\n";

    while(true) {
        vector<int> msg;
        mt19937 rng;
        rng.seed(random_device()());
        uniform_int_distribution<mt19937::result_type> dist(1, 2);
        unsigned long r = dist(rng);
        if (r == 1) {
            msg.push_back(1);

            rng.seed(random_device()());
            uniform_int_distribution<mt19937::result_type> dist(1, 1000);
            unsigned long r1 = dist(rng);

            msg.push_back(r1);
            cout << "Process " << id << " sends message " << r1 << " to leader \n";
            send(msg, leader);

        } else if (r == 2) {
            msg.push_back(2);
            msg.push_back(id);
            send(msg , leader);
            vector<int> reply = receive(leader);
            if (reply[0] == 4) {
                leader_election(id, size);
            }
            else if (reply[0] == 3)
            {
                if(reply[1] == -1)
                    cout<<" No information available for process "<<id<<"\n";
                else
                    cout << "Process " << id << " receives information from leader \n";
            }
            else
                cout<<"This shouldnt come";
        }
        int flag;
        MPI_Iprobe(leader, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);

        if (flag && receive(leader)[0] == 4)
            break;

    }

    leader_election(id, size);
}

int main ( int argc, char *argv[] )
{
    int id;
    int ierr;
    int p;

    double wtime;
    ierr = MPI_Init ( &argc, &argv );
    if ( ierr != 0 )
    {
        cout << "\n";
        cout << "HELLO_MPI - Fatal error!\n";
        cout << "  MPI_Init returned nonzero ierr.\n";
        exit ( 1 );
    }

    ierr = MPI_Comm_size ( MPI_COMM_WORLD, &p );
    ierr = MPI_Comm_rank ( MPI_COMM_WORLD, &id );
    leader_election(id , p);

}
