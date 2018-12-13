#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <pthread.h>

using namespace std;

struct packet
{
    string type;
    int arrival_time;
    int travel_time;
    string route;
    int id;
    bool flag; // to ensure a waiting packet is counted only once
};

static pthread_mutex_t bsem;
static pthread_mutex_t asem;
static pthread_cond_t AB = PTHREAD_COND_INITIALIZER;
static pthread_cond_t BA = PTHREAD_COND_INITIALIZER;
static string CHANNEL;
static int CAPACITY;
static int usage = 0; // will keep track of number of packets using channel for capacity checking
static bool packetsProcessing = true; // will be used to monitor channel thread loop (false means thread exits)

// Counters for packets that had to wait
static int wait_ab_ftp = 0;
static int wait_ab_http = 0;
static int wait_ab_ssh = 0;
static int wait_ab_smtp = 0;

static int wait_ba_ftp = 0;
static int wait_ba_http = 0;
static int wait_ba_ssh = 0;
static int wait_ba_smtp = 0;

// These variables are used in combination with packetsProcessing to see if channel needs to be closed
static int final_count = 0;
static int counter = 0;

// Switches the channel back and forth
void *channel_thread(void *family_void_ptr)
{
    int i = 0;

	while(packetsProcessing)
	{
		pthread_mutex_lock(&bsem);

		switch(i)
		{
			case 0:
                	CHANNEL = "AB";
                	pthread_cond_signal(&AB); // wake up first item in AB queue
				break;
				
			case 1:
                	CHANNEL = "NONE";
                		break;
				
			case 2:
                	CHANNEL = "BA";
                	pthread_cond_signal(&BA); // wake up first item in BA queue
                		break;
				
			case 3:
                	CHANNEL = "NONE";
               			break;
		}

		cout << "The current direction of the channel is: " << CHANNEL << endl;
		pthread_mutex_unlock(&bsem);

		sleep(5);

		i = (i+1) % 4;

	}

	return NULL;
}

void *packet_thread(void *family_void_ptr)
{
    struct packet threadPackets;
    threadPackets.arrival_time = ((struct packet *)family_void_ptr)->arrival_time;
    threadPackets.travel_time = ((struct packet *)family_void_ptr)->travel_time;
    threadPackets.type = ((struct packet *)family_void_ptr)->type;
    threadPackets.route = ((struct packet *)family_void_ptr)->route;
    threadPackets.id = ((struct packet *)family_void_ptr)->id;

    // Arrival message
    cout << "Packet #" << threadPackets.id << " (" << threadPackets.type << ")"
         << " from " << threadPackets.route << " has arrived to the channel" << endl;

    pthread_mutex_lock(&asem);

        // Make packets wait if they are not the same route, or capacity is reached
        while(threadPackets.route != CHANNEL || usage == CAPACITY)
        {
            if(threadPackets.route == "AB")
            {
                // Only want to count packets that had to wait because of full capacity
                if(usage == CAPACITY)
                {
                    // Count the wait packets for A to B
                    if(threadPackets.type == "FTP" && threadPackets.flag == false)
                    {
                        wait_ab_ftp += 1;
                        threadPackets.flag = true;
                    }

                    else if(threadPackets.type == "SSH" && threadPackets.flag == false)
                    {
                        wait_ab_ssh += 1;
                        threadPackets.flag = true;
                    }

                    else if(threadPackets.type == "HTTP" && threadPackets.flag == false)
                    {
                        wait_ab_http += 1;
                        threadPackets.flag = true;
                    }

                    else if(threadPackets.type == "SMTP" && threadPackets.flag == false)
                    {
                        wait_ab_smtp += 1;
                        threadPackets.flag = true;
                    }
                }

                // Force AB routed thread to wait until we are ready to wake it
                pthread_cond_wait(&AB, &asem);
            }

            if(threadPackets.route == "BA")
            {
                // Only want to count packets that had to wait because of full capacity
                if(usage == CAPACITY)
                {
                    // Count the wait packets for B to A
                    if(threadPackets.type == "FTP" && threadPackets.flag == false)
                    {
                        wait_ba_ftp += 1;
                        threadPackets.flag = true;
                    }

                    else if(threadPackets.type == "SSH" && threadPackets.flag == false)
                    {
                        wait_ba_ssh += 1;
                        threadPackets.flag = true;
                    }

                    else if(threadPackets.type == "HTTP" && threadPackets.flag == false)
                    {
                        wait_ba_http += 1;
                        threadPackets.flag = true;
                    }

                    else if(threadPackets.type == "SMTP" && threadPackets.flag == false)
                    {
                        wait_ba_smtp += 1;
                        threadPackets.flag = true;
                    }
                }

                // Force BA routed thread to wait until we are ready to wake it
                pthread_cond_wait(&BA, &asem);
            }
        }

        // Packet is now using the channel message
        usage ++;

        cout << "Packet #" << threadPackets.id << " (" << threadPackets.type << ")"
             << " from " << threadPackets.route << " is using the channel" << endl;

    pthread_mutex_unlock(&asem);

    // Sleep for packets travel time, needs to be oustide of critical section
    sleep(threadPackets.travel_time);

    pthread_mutex_lock(&asem);

        // Decrement capacity as the packet has now left the channel
        usage--;

        // If a packet makes it this far, it has traveled the channel and exits
        cout << "Packet #" << threadPackets.id << " (" << threadPackets.type << ")"
             << " from " << threadPackets.route << " exits the channel" << endl;

        final_count += 1; // to be used in checking when to close the channel thread

        // Wake up sleeping packets
        if(CHANNEL == "AB")
        {
            pthread_cond_signal(&AB);
        }

        else if(CHANNEL == "BA")
        {
            pthread_cond_signal(&BA);
        }

    pthread_mutex_unlock(&asem);

    // Check if channel thread should close. When every packet makes it through successfully, flag is set to false
    if(counter == final_count)
    {
        packetsProcessing = false;
    }

    return NULL;
}

int main()
{
    struct packet myPackets[50];
    string type, route;
    int arrival, travel;

    pthread_t c_thread; // channel thread
    pthread_t p_thread; // packet thread
    pthread_mutex_init(&bsem, NULL); // Initialize access to 1
    pthread_mutex_init(&asem, NULL); // Initialize access to 1

    // Counters for sent packets
    int sent_ab_ftp = 0;
    int sent_ab_http = 0;
    int sent_ab_ssh = 0;
    int sent_ab_smtp = 0;

    int sent_ba_ftp = 0;
    int sent_ba_http = 0;
    int sent_ba_ssh = 0;
    int sent_ba_smtp = 0;

    cin >> CAPACITY;

    cout << "The channel capacity is: " << CAPACITY << endl << endl;

    while(cin >> type >> arrival >> route >> travel)
    {
        packet newpacket;
        newpacket.type = type;
        newpacket.arrival_time = arrival;
        newpacket.travel_time = travel;
        newpacket.route = route;
        newpacket.id = counter + 1;
        newpacket.flag = false;

        // Packets going from A to B
        if(type == "FTP" && route == "AB")
        {
            sent_ab_ftp += 1;
        }

        else if(type == "SSH" && route == "AB")
        {
            sent_ab_ssh += 1;
        }

        else if(type == "HTTP" && route == "AB")
        {
            sent_ab_http += 1;
        }

        else if(type == "SMTP" && route == "AB")
        {
            sent_ab_smtp += 1;
        }

        // Packets going from B to A
        else if(type == "FTP" && route == "BA")
        {
            sent_ba_ftp += 1;
        }

        else if(type == "SSH" && route == "BA")
        {
            sent_ba_ssh += 1;
        }

        else if(type == "HTTP" && route == "BA")
        {
            sent_ba_http += 1;
        }

        else if(type == "SMTP" && route == "BA")
        {
            sent_ba_smtp += 1;
        }

        myPackets[counter] = newpacket;
        counter++;
    }

    // Create channel thread
    if(pthread_create(&c_thread, NULL, channel_thread,(void *)NULL))
	{
		fprintf(stderr, "Error creating thread\n");
		return 1;
	}

    for (int i = 0; i < counter; i++)
    {
        // Sleep for the time it would take packet to reach the channel
        sleep(myPackets[i].arrival_time);

        // Create packet thread
        if(pthread_create(&p_thread, NULL, packet_thread,(void *)&myPackets[i]))
        {
            fprintf(stderr, "Error creating thread\n");
    		return 1;
        }
    }

    // Wait for other threads to finish
    pthread_join(c_thread, NULL);

    cout << endl;

    // Display packets transmitted
    cout << "Packets transmitted:" << endl;
    cout << "\tA to B:" << endl;
    cout << "\t\tFTP: " << sent_ab_ftp << " SSH: " << sent_ab_ssh
         << " HTTP: " << sent_ab_http << " SMTP: " << sent_ab_smtp;

    cout << endl;

    cout << "\tB to A:" << endl;
    cout << "\t\tFTP: " << sent_ba_ftp << " SSH: " << sent_ba_ssh
         << " HTTP: " << sent_ba_http << " SMTP: " << sent_ba_smtp;

    cout << endl << endl;

    // Display packets that waited
    cout << "Packets that waited:" << endl;
    cout << "\tA to B:" << endl;
    cout << "\t\tFTP: " << wait_ab_ftp << " SSH: " << wait_ab_ssh
         << " HTTP: " << wait_ab_http << " SMTP: " << wait_ab_smtp;

    cout << endl;

    cout << "\tB to A:" << endl;
    cout << "\t\tFTP: " << wait_ba_ftp << " SSH: " << wait_ba_ssh
         << " HTTP: " << wait_ba_http << " SMTP: " << wait_ba_smtp;

    cout << endl;
    return 0;
}
