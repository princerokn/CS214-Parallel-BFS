//Prince's Prefix sum
int* prefix_sum(int arr[], int start, int end) {
    int n = end-start+1;
    int* arr1 = new int[n/2];
    int* res = new int[n];
    res[0] = arr[0];
    if(n > 1) {
        if (n <= 100) { //coarsening in prefix_sum
            for(int i = 0; i < n/2; i++){
                arr1[i] = arr[2 * i] + arr[2 * i + 1];
            }
            int *arr2 = prefix_sum(arr1, start, start + (end - start) / 2);
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
            int *arr2 = prefix_sum(arr1, start, start + (end - start) / 2);
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
//Combined packing and flattening
//Prince' Flatten parallel
//work O(n), span O(log n)
int* flattenPar(int* curFrontier, int curFrontSize, int* offset, int* E, bool* visited, int& nextFrontierSize){
    // cout<<endl<<curFrontSize<<" /t"<< nextFrontierSize;
    int* sizeOfArrays = new int[curFrontSize]; //This array will store the size of each neighbor of nodes in frontier
    int* offsetForArrays = new int[curFrontSize]; //This array will offset values for arrays in frontier.
    //This will determine size of neighbor of each node in frontier and store.
    parallel_for (0, curFrontSize, [&] (int i) {
                      sizeOfArrays[i] = offset[curFrontier[i]+1] - offset[curFrontier[i]];
                  }
    );
    offsetForArrays = prefix_sum(sizeOfArrays, 0, curFrontSize); //This will set the offset values in the array

    int sizeofflattenedArray = offsetForArrays[curFrontSize-1];
    int* flattenedArray = new int[sizeofflattenedArray];       //array with original values
    int* flattenedArrFlag = new int[sizeofflattenedArray];  //flag array with 0 or 1
    parallel_for (0, sizeofflattenedArray, [&] (int i) {flattenedArrFlag[i] = 0;}); // make all flags false initially.

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
    prefixSumArr = prefix_sum(flattenedArrFlag,0,sizeofflattenedArray);
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

//Prince's BFS function
void BFS(int n, int m, int* offset, int* E, int s, int* dist) {
    bool* visited = new bool [n]; //This is for flag visited or not.
    int* curFrontier = new int[n];
    int curFrontSize = 0;
    int nextFrontierSize = 0;
    parallel_for (0, n, [&] (int i) {dist[i] = -1;});   //initialize all the elements in dist[i] to -1
    parallel_for (0, n, [&] (int i) {visited[i] = false;}); // make all elements not visited.
    int curDistance = 0;
    dist[s] = curDistance;   // source
    visited[s] = true; //making source visited
    //This will create 1st frontier
    curFrontier[curFrontSize++] = s;
    //Flattened array
    while(curFrontSize > 0){
        parallel_for (0, curFrontSize, [&] (int i) {dist[curFrontier[i]] = curDistance;});
        int* nextFrontier = flattenPar(curFrontier,curFrontSize,offset, E, visited, nextFrontierSize);
        delete[] curFrontier;
        curFrontier = nextFrontier;
        curFrontSize = nextFrontierSize;
        curDistance++;
    }
}

