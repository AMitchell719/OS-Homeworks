#include <iostream>
#include <unistd.h>
#include <algorithm>
#include <sys/wait.h>

using namespace std;

struct process
{
    int id;
    int exectime;
    int priority;
    int totaltime;
    int sortFlag;
    int isSplit;
};

bool comparison(process a, process b)
{
    if(a.priority == b.priority)
    {
        return (a.sortFlag < b.sortFlag);
    }

    return a.priority < b.priority;
}

bool comparison2(process a, process b)
{
    return a.priority < b.priority;
}

// Will try this function on array to sort
void scheduler(process proc[], int n)
{
    // array, array + len, sort_fn
    sort(proc, proc + n, comparison);
}

void scheduler2(process proc[], int n)
{
    // array, array + len, sort_fn
    sort(proc, proc + n, comparison2);
}

int main()
{
    // totaltime will track the original exectime in the file for displaying purposes when split
    int pid, quantum, exectime, priority, totaltime, id;
    pid_t p;
    struct process myProcess[10];
    id = 0;
    pid = 0;

    // Program grabs input from file redirection. Grab quantum first
    cin >> quantum;
    cout << "The quantum is: " << quantum << endl << endl;

    // Grab remaining file contents and store into struct
    while(cin >> exectime >> priority)
    {
        process newProcess;
        newProcess.exectime = exectime;
        newProcess.priority = priority;
        newProcess.isSplit = false;
        newProcess.id = id;
        newProcess.totaltime = exectime;
        newProcess.sortFlag = 0;

        // Testing if we need to split a process into two
        if(newProcess.exectime - quantum >= 1)
        {
            newProcess.exectime = quantum;
            newProcess.isSplit = true;
            newProcess.id = id;
            newProcess.totaltime = exectime;
            newProcess.sortFlag = 0;

            // Spawn second process, assign it the leftover exectime and same priority
            process secondProcess;
            secondProcess.exectime = exectime - quantum;
            secondProcess.priority = priority;
            secondProcess.isSplit = false;
            secondProcess.id = id;
            secondProcess.totaltime = exectime; // keeps track of exectime before split for display purposes
            secondProcess.sortFlag = 1;

            // Store processes in array
            myProcess[pid] = newProcess;
            myProcess[pid+1] = secondProcess;

            // Increment by 2 since we now have 2 processes to store in array (pid is array size counter)
            pid = pid + 2;

            // Keeps track of actual process id's since pid needs to be incremented by more than one
            id++;
        }

        // Otherwise store the process as is into the array
        else
        {
            myProcess[pid] = newProcess;
            id++;
            pid++;
        }
    }

    // Begin sorting
    scheduler2(myProcess, pid);
    scheduler(myProcess, pid);

    // Display scheduling queue contents
    cout << "Scheduling Queue:" << endl;
    for (int i = 0; i < pid; i++)
    {
        cout << "Process ID: " << myProcess[i].id << endl;
        cout << "Execution Time: " << myProcess[i].exectime << endl;
        cout << "Priority: " << myProcess[i].priority << endl << endl;
    }

    // Begin scheduling of forking
    for (int i = 0; i < pid; i++)
    {

        if((p = fork())==0)
        {
            // Print the information, but check if it's split
            if(myProcess[i].isSplit == false)
            {
                cout << "PID " << myProcess[i].id << ": Execution Time: " << myProcess[i].exectime <<
                ", Priority: " << myProcess[i].priority << endl;
                cout << "Process " << myProcess[i].id << " has finished executing" << endl << endl;
            }

            // Print different information for the process that isn't split
            else
            {
                cout << "PID " << myProcess[i].id << ": Execution Time: " << myProcess[i].exectime <<
                ", Priority: " << myProcess[i].priority << endl << endl;
            }
            // Exit out of child
            _exit(0);
        }

        wait(NULL);
    }

    cin.get();
    sleep(10);

    return 0;
}
