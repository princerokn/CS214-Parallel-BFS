//Prince's Prefix sum
int* scan(int arr[], int start, int end) {
    int n = end - start + 1;
    int* arr1 = new int[n/2];
    int* res = new int[n];
    res[0] = arr[0];
    if(n > 1) {
        if (n <= 100) { //coarsening in scan
            for(int i = 0; i < n/2; i++){
                arr1[i] = arr[2 * i] + arr[2 * i + 1];
            }
            int *arr2 = scan(arr1, start, start + (end - start) / 2);
            for(int i = 1; i < n; i++){
                if (i % 2 == 1)
                    res[i] = arr2[i / 2];
                else
                    res[i] = arr2[(i - 1) / 2] + arr[i];
            }
            parallel_for(1, n, [&](int i) {
                if (i % 2 == 1)
                    res[i] = arr2[i / 2];
                else
                    res[i] = arr2[(i - 1) / 2] + arr[i];
            });
            delete[] arr2;
        }
        else{
            parallel_for(0, n / 2, [&](int i) { arr1[i] = arr[2 * i] + arr[2 * i + 1]; });
            int *arr2 = scan(arr1, start, start + (end - start) / 2);
            parallel_for(1, n, [&](int i) {
                if (i % 2 == 1)
                    res[i] = arr2[i / 2];
                else
                    res[i] = arr2[(i - 1) / 2] + arr[i];
            });
            delete[] arr2;
        }
    }
    delete[] arr1;
    return res;
}

//Prince's Packing parallel
//Prince' Flatten parallel
//Combined packing and flattening
int* flattenandPack(int* curFrontier, int curFrontSize, int* offset, int* E, bool* visited, int& nextFrontierSize){
    int* sizeOfArrays = new int[curFrontSize]; //This array will store the size of each neighbor of nodes in frontier
    int* offsetForArrays = new int[curFrontSize]; //This array will offset values for arrays in frontier.
    //This will determine size of neighbor of each node in frontier and store.
    parallel_for (0, curFrontSize, [&] (int i) {
                      sizeOfArrays[i] = offset[curFrontier[i]+1] - offset[curFrontier[i]];
                  }
    );
    offsetForArrays = scan(sizeOfArrays, 0, curFrontSize); //This will set the offset values in the array

    int sizeofflattenedArray = offsetForArrays[curFrontSize-1];
    int* flattenedArray = new int[sizeofflattenedArray];       //array with original values
    int* flattenedArrFlag = new int[sizeofflattenedArray];  //flag array with 0 or 1
    parallel_for (0, sizeofflattenedArray, [&] (int i) {flattenedArrFlag[i] = 0;}); // make all flags false initially.
    //Flatten the array and store it
    parallel_for (0, curFrontSize, [&] (int i) {
                      int off = (i>0)?offsetForArrays[i-1]:0;
                      int node = curFrontier[i];
                      int location = offset[node];
                      parallel_for(0,sizeOfArrays[i], [&] (int j) {
                          int y = E[j+location];
                          flattenedArray[off+j] = y;
                          if(!visited[y] && __sync_bool_compare_and_swap(&visited[y], false, true))
                              flattenedArrFlag[off+j] = 1;
                      });
                  }
    );
    // flattenedArray contains elements after being flattened
    // flattenedArrFlag contains elements after CAS
    delete[] sizeOfArrays;
    delete[] offsetForArrays;
    //Flattening starts
    int* prefixSumArr = new int[sizeofflattenedArray];
    prefixSumArr = scan(flattenedArrFlag,0,sizeofflattenedArray);
    nextFrontierSize = prefixSumArr[sizeofflattenedArray-1];
    int* packedArray = new int[nextFrontierSize];
    //This will pack the flattened array.
    parallel_for (0, sizeofflattenedArray, [&] (int i) {
                      if(flattenedArrFlag[i])
                          packedArray[prefixSumArr[i] - 1] = flattenedArray[i];
                  }
    );
    delete[] prefixSumArr;
    delete[] flattenedArrFlag;
    return packedArray;
}
//Prince's Dense Backward function
bool* denseMode(int n, bool* denseFrontier, int* offset, int* E, bool* visited, int& nextFrontierSize){
    bool* nextFrontier = new bool[n];
    int count = 0;
    parallel_for (0, n, [&] (int i) {nextFrontier[i] = 0;}); // set initial value of new Frontier
    for (int i = 0; i < n; i++) {
        if(!visited[i]){
            //check if its neighbour is in the frontier
            //if yes the add it to the new frontier
            for(int j = offset[i]; j < offset[i+1]; j++){
                if(denseFrontier[E[j]]){
                    visited[i] = true;
                    nextFrontier[i] = 1;
                    count++;
                    break;
                }
            }
        }
    }
    nextFrontierSize = count;
    return nextFrontier;
}
//Prince's BFS function
void BFS(int n, int m, int* offset, int* E, int s, int* dist) {
    bool *visited = new bool[n]; //This is flag to check visited or not.
    int *curFrontier = new int[n];
    int curFrontSize = 0;
    int nextFrontierSize = 0;
    parallel_for(0, n, [&](int i) { dist[i] = -1; });   //initialize all the elements in dist[i] to -1
    parallel_for(0, n, [&](int i) { visited[i] = false; }); //make all elements not visited.
    int curDistance = 0;
    dist[s] = curDistance;   // source
    visited[s] = true; //making source visited
    //This will create 1st frontier
    curFrontier[curFrontSize++] = s;
    //0 means sparse and 1 means dense
    bool mode = 0;
    bool todo = 0;
    int *nextFrontier;
    bool* denseFrontier = new bool[n];
    bool* nextDenseFrontier = new bool[n];
    int* prefixSumArr = new int[n];
    int* denseFrontierInt = new int[n];
    while (curFrontSize > 0) {
        curDistance++;
        if(curFrontSize <= n/12+1)
            todo = 0;
        else
            todo = 1;
        if(mode != todo){
            //switch mode logic.
            if(todo){
                //switch to dense, make curFrontier of n sized boolean
                parallel_for (0, n, [&] (int i) {denseFrontier[i] = 0;});
                parallel_for (0, curFrontSize, [&] (int i) {denseFrontier[curFrontier[i]] = true;}); // setting the bits for cur dense Frontier
            }
            else{
                //switch to sparse mode, set a packed current frontier.
                //this switch happen only when we are moving from dense to sparse mode
                //Since our current Frontier is in dense Frontier format, we need to obtain a packed cur Frontier
                parallel_for (0, n, [&] (int i) {denseFrontierInt[i] = denseFrontier[i];});
                prefixSumArr = scan(denseFrontierInt, 0, n);
                int ptr = 0;
                if(prefixSumArr[0] == 1)
                    curFrontier[ptr++] = 0;
                for(int i = 1; i < n; i++) {
                    if (prefixSumArr[i] != prefixSumArr[i - 1])
                        curFrontier[ptr++] = i;
                }
            }
            mode = todo;
        }
        if (!mode) {
            //Run in Sparse Mode
            nextFrontier = flattenandPack(curFrontier, curFrontSize, offset, E, visited, nextFrontierSize);
            curFrontier = nextFrontier;
            parallel_for(0, nextFrontierSize, [&](int i) { dist[curFrontier[i]] = curDistance; }); //Fill the distance array
        } else {
            //Run in dense Mode
            nextDenseFrontier = denseMode(n, denseFrontier, offset, E,visited, nextFrontierSize);
            denseFrontier = nextDenseFrontier;
            parallel_for (0, n, [&] (int i) {if(denseFrontier[i]) dist[i] = curDistance;});
        }
        curFrontSize = nextFrontierSize;
    }
}
