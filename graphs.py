import matplotlib.pyplot as plt
import subprocess
import time
import numpy as np
import os

def plotGraphsAll(TESTS, nb_runcalls):

    LIST_Y_ours_dyn = [[] for _ in TESTS]
    LIST_Y_ours_static = [[] for _ in TESTS]
    LIST_Y_posix = [[] for _ in TESTS]
    LIST_X = [[] for _ in TESTS]
    
    subprocess.run(['rm','-f','libthread.so', 'thread.o'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    subprocess.run(['rm','-rf','install'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    subprocess.run(['make'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    print("Computing tests for DYNAMIC ",end="")
    for id, test_name in enumerate(TESTS):
        max_threads = 400
        step = 5
        if test_name in ["51-fibonacci"]:
            max_threads = 21
            step = 1
        elif test_name in ["61-mutex"]:
            max_threads //= 4
        elif test_name in ["62-mutex"]:
            max_threads //= 8
        LIST_X[id] = range(1, max_threads, step)

        #_______________Our Thread Library___________________
        env = {'LD_LIBRARY_PATH': '$LD_LIBRARY_PATH:./install/lib'}
        for i in LIST_X[id]:
            Y = []
            for k in range(nb_runcalls):
                start_time = time.time()
                if test_name in ["31-switch-many","32-switch-many-join", "33-switch-many-cascade"]:
                    subprocess.run(['./install/bin/'+test_name, "10", str(i), '--flag'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, env=env)
                else:
                    subprocess.run(['./install/bin/'+test_name, str(i), '--flag'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, env=env)
                our_time = time.time() - start_time
                Y.append(our_time)
            LIST_Y_ours_dyn[id].append(np.mean(Y))
        print(".", end="", flush=True)
    
    
    subprocess.run(['rm','-f','libthread.so', 'thread.o', 'install/lib/libthread.so'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    subprocess.run(['make', 'static'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    print("\nComputing tests for STATIC ",end="")
    for id, test_name in enumerate(TESTS):
        #_______________Our Thread Library STATIC___________________
        env = {'LD_LIBRARY_PATH': '$LD_LIBRARY_PATH:./install/lib'}
        for i in LIST_X[id]:
            Y = []
            for k in range(nb_runcalls):
                start_time = time.time()
                if test_name in ["31-switch-many","32-switch-many-join", "33-switch-many-cascade"]:
                    subprocess.run(['./install/bin/'+test_name, "10", str(i), '--flag'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, env=env)
                else:
                    subprocess.run(['./install/bin/'+test_name, str(i), '--flag'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, env=env)
                our_time = time.time() - start_time
                Y.append(our_time)
            LIST_Y_ours_static[id].append(np.mean(Y))
        print(".", end="", flush=True)
    
    print("\nComputing tests for POSIX ",end="")
    subprocess.run(['rm','-rf','install'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    subprocess.run(['make', 'pthreads'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    for id, test_name in enumerate(TESTS):
        #_____________ POSIX Threads______________________
        for i in LIST_X[id]:
            Y = []
            for k in range(nb_runcalls):
                start_time = time.time()
                subprocess.run(['./install/bin/'+test_name, str(i), '--flag'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                posix_time = time.time() - start_time
                Y.append(posix_time)
            LIST_Y_posix[id].append(np.mean(Y))
        print(".", end="", flush=True)
    

    print("\nComputing graphs")
    for id, test_name in enumerate(TESTS):
        #_____________ The Graph making___________________
        X = LIST_X[id]
        plt.plot(X, LIST_Y_ours_dyn[id], marker='o')
        plt.plot(X, LIST_Y_ours_static[id], marker='s')
        plt.plot(X, LIST_Y_posix[id], marker='+')
        
        plt.title("Comparison of performance on "+test_name +" with "+str(nb_runcalls)+" runcalls")
        plt.xlabel("num of threads")
        plt.ylabel("Time in ms")
        plt.legend(["Our library (DYNAMIC)", "Our library (STATIC)", "POSIX"])
        
        if not os.path.exists('graphs'):
            os.makedirs('graphs')
            
        graph_path = os.path.join('graphs', test_name + '.png')
        plt.savefig(graph_path)
        
        plt.close()


def main():
    RUNCALLS = 5
    plotTests = ["21-create-many", "22-create-many-recursive", "23-create-many-once","31-switch-many","32-switch-many-join", "33-switch-many-cascade","51-fibonacci","52-sum-list","61-mutex","62-mutex"]
    plotGraphsAll(plotTests, RUNCALLS)

    
main()