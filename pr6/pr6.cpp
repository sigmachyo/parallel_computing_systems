#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <mpi.h>

using namespace std;

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        cout << "    PARALLEL MINIMUM FINDING WITH MPI" << endl;
        cout << "      (Collective Operations)" << endl;
        cout << "Program running on " << size << " processes" << endl << endl;
    }

    vector<int> global_array;
    vector<int> local_array;
    int N = 0;

    if (rank == 0) {
        cout << "Enter array size N: ";
        cin >> N;

        srand(static_cast<unsigned>(time(nullptr)));
        global_array.resize(N);

        cout << "\nGenerated array (" << N << " elements):" << endl;
        for (int i = 0; i < N; i++) {
            global_array[i] = rand() % 1000;
            cout << global_array[i];
            if (i < N - 1) cout << " ";
            if ((i + 1) % 10 == 0) cout << endl;
        }
        if (N % 10 != 0) cout << endl;

        int seq_min = global_array[0];
        for (int i = 1; i < N; i++) {
            if (global_array[i] < seq_min) {
                seq_min = global_array[i];
            }
        }
        cout << "\nSequential minimum: " << seq_min << endl << endl;
    }

    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int base_size = N / size;
    int remainder = N % size;
    int local_size = base_size + (rank < remainder ? 1 : 0);

    vector<int> send_counts(size);
    vector<int> displacements(size);

    if (rank == 0) {
        int displacement = 0;
        for (int i = 0; i < size; i++) {
            send_counts[i] = base_size + (i < remainder ? 1 : 0);
            displacements[i] = displacement;
            displacement += send_counts[i];
        }

        cout << "Array distribution between processes:" << endl;
        for (int i = 0; i < size; i++) {
            cout << "Process " << i << ": " << send_counts[i]
                 << " elements (offset: " << displacements[i] << ")" << endl;
        }
        cout << endl;
    }

    MPI_Bcast(send_counts.data(), size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(displacements.data(), size, MPI_INT, 0, MPI_COMM_WORLD);

    local_array.resize(local_size);

    MPI_Scatterv(
        (rank == 0) ? global_array.data() : nullptr,
        send_counts.data(),
        displacements.data(),
        MPI_INT,
        local_array.data(),
        local_size,
        MPI_INT,
        0,
        MPI_COMM_WORLD
    );

    int local_min = (local_size > 0) ? local_array[0] : numeric_limits<int>::max();
    for (int i = 1; i < local_size; i++) {
        if (local_array[i] < local_min) {
            local_min = local_array[i];
        }
    }

    cout << "Process " << rank << ": local fragment size = " << local_size
         << ", local minimum = " << local_min << endl;

    int global_min;
    MPI_Reduce(
        &local_min,
        &global_min,
        1,
        MPI_INT,
        MPI_MIN,
        0,
        MPI_COMM_WORLD
    );

    vector<int> all_local_mins;
    if (rank == 0) {
        all_local_mins.resize(size);
    }

    MPI_Gather(
        &local_min,
        1,
        MPI_INT,
        (rank == 0) ? all_local_mins.data() : nullptr,
        1,
        MPI_INT,
        0,
        MPI_COMM_WORLD
    );

    if (rank == 0) {
        cout << "\n              FINAL RESULTS" << endl;
        cout << "Global minimum found: " << global_min << endl << endl;

        cout << "Verification:" << endl;
        cout << "Local minima from all processes:" << endl;
        for (int i = 0; i < size; i++) {
            cout << "  Process " << i << ": " << all_local_mins[i];
            if (all_local_mins[i] == global_min) {
                cout << "  <-- GLOBAL MINIMUM";
            }
            cout << endl;
        }

        if (global_array.size() > 0) {
            int actual_min = global_array[0];
            for (int i = 1; i < N; i++) {
                if (global_array[i] < actual_min) {
                    actual_min = global_array[i];
                }
            }

            cout << "\nVerification result:" << endl;
            cout << "  Sequential min: " << actual_min << endl;
            cout << "  Parallel min:   " << global_min << endl;

            if (actual_min == global_min) {
                cout << "  ✓ Results match!" << endl;
            } else {
                cout << "  ✗ Results differ!" << endl;
            }
        }

        cout << "\nProgram completed successfully." << endl;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        cout << "\nPress Enter to exit..." << endl;
        cin.ignore();
        cin.get();
    }

    MPI_Finalize();

    return 0;
}