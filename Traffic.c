#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h> // For sleep()
#include <mysql/mysql.h>
#include <limits.h> // For INT_MAX

#define BASE_GREEN_TIME 30

typedef enum {
    T_INTERSECTION,
    PLUS_INTERSECTION
} IntersectionType;

typedef struct Lane {
    int vehicleCount;
    int greenTime;
    int redTime;
    int priority;
    int waitingTime;
    int emergencyVehicle; // 0: No, 1: Yes
    struct Lane* next;
    struct Lane* prev;
    int laneNumber;
} Lane;

typedef struct {
    IntersectionType type;
    int laneCount;
    Lane* head;
    Lane* tail;
} Intersection;

// MySQL Connection Setup
MYSQL* connectDB() {
    MYSQL* conn = mysql_init(NULL);

    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        exit(EXIT_FAILURE);
    }

    // Replace these values with your MySQL server details
    if (mysql_real_connect(conn, "localhost", "root", "", "traffic_management", 3306, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(EXIT_FAILURE);
    }

    return conn;
}


// Fetch lane data from MySQL database
void fetchLaneData(Intersection* intersection) {
    MYSQL* conn = connectDB();
    MYSQL_RES* res;
    MYSQL_ROW row;

    const char* query = "SELECT lane_number, vehicle_count, emergency_vehicle FROM lanes";
    if (mysql_query(conn, query)) {
        fprintf(stderr, "SELECT failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(EXIT_FAILURE);
    }

    res = mysql_store_result(conn);
    if (res == NULL) {
        fprintf(stderr, "mysql_store_result() failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(EXIT_FAILURE);
    }

    Lane* currentLane = intersection->head;
    while ((row = mysql_fetch_row(res)) != NULL) {
        int laneNumber = atoi(row[0]);
        currentLane->vehicleCount = atoi(row[1]);
        currentLane->emergencyVehicle = atoi(row[2]);
        currentLane = currentLane->next;
    }

    mysql_free_result(res);
    mysql_close(conn);
}

// Calculate green time for each lane
void calculateGreenTime(Lane* lane, int totalVehicles, int laneCount) {
    if (lane->emergencyVehicle) {
        lane->greenTime = INT_MAX; // Assign maximum green time for emergency vehicles
    } else {
        lane->greenTime = (lane->vehicleCount * 60) / totalVehicles; // Example calculation
    }
}

// Calculate red time for each lane
void calculateRedTime(Lane* lane, int totalTime) {
    lane->redTime = totalTime - lane->greenTime;
}

// Calculate priority based on vehicle count and emergency status
void calculatePriority(Lane* lane) {
    if (lane->emergencyVehicle) {
        lane->priority = INT_MAX; // Assign maximum priority to emergency vehicles
    } else {
        lane->priority = lane->vehicleCount; // Example priority based on vehicle count
    }
}

// Create Intersection
Intersection* createIntersection(IntersectionType type) {
    Intersection* intersection = (Intersection*)malloc(sizeof(Intersection));
    if (!intersection) {
        fprintf(stderr, "Memory allocation failed for Intersection.\n");
        exit(EXIT_FAILURE);
    }

    intersection->type = type;
    intersection->head = NULL;
    intersection->tail = NULL;

    switch (type) {
    case T_INTERSECTION:
        intersection->laneCount = 3;
        break;
    case PLUS_INTERSECTION:
        intersection->laneCount = 4;
        break;
    }

    for (int i = 0; i < intersection->laneCount; i++) {
        Lane* newLane = (Lane*)malloc(sizeof(Lane));
        if (!newLane) {
            fprintf(stderr, "Memory allocation failed for Lane.\n");
            exit(EXIT_FAILURE);
        }
        newLane->vehicleCount = 0;
        newLane->greenTime = 0;
        newLane->redTime = 0;
        newLane->priority = 0;
        newLane->waitingTime = 0;
        newLane->emergencyVehicle = 0;
        newLane->laneNumber = i + 1;

        if (intersection->tail) {
            intersection->tail->next = newLane;
            newLane->prev = intersection->tail;
        } else {
            intersection->head = newLane;
            newLane->prev = NULL;
        }

        intersection->tail = newLane;
    }

    // Making it circular
    intersection->tail->next = intersection->head;
    intersection->head->prev = intersection->tail;

    return intersection;
}

// Destroy Intersection
void destroyIntersection(Intersection* intersection) {
    if (intersection == NULL) return;

    Lane* currentLane = intersection->head;
    Lane* nextLane;

    do {
        nextLane = currentLane->next;
        free(currentLane);
        currentLane = nextLane;
    } while (currentLane != intersection->head);

    free(intersection);
}

// Calculate Signal Timing
void calculateSignalTiming(Intersection* intersection) {
    int totalVehicles = 0;
    int totalTime = 0;

    fetchLaneData(intersection); // Fetch real-time lane data from the database.

    Lane* currentLane = intersection->head;
    do {
        totalVehicles += currentLane->vehicleCount;
        currentLane = currentLane->next;
    } while (currentLane != intersection->head);

    currentLane = intersection->head;
    do {
        calculateGreenTime(currentLane, totalVehicles, intersection->laneCount);
        totalTime += currentLane->greenTime;
        currentLane = currentLane->next;
    } while (currentLane != intersection->head);

    currentLane = intersection->head;
    do {
        calculateRedTime(currentLane, totalTime);
        calculatePriority(currentLane);
        currentLane = currentLane->next;
    } while (currentLane != intersection->head);
}

// Display Signal Timing (Dummy implementation, you should adjust as needed)
void displaySignalTiming(Intersection* intersection) {
    Lane* currentLane = intersection->head;
    do {
        printf("Lane %d:\n", currentLane->laneNumber);
        printf("  Vehicle Count: %d\n", currentLane->vehicleCount);
        printf("  Emergency Vehicle: %d\n", currentLane->emergencyVehicle);
        printf("  Green Time: %d\n", currentLane->greenTime);
        printf("  Red Time: %d\n", currentLane->redTime);
        printf("  Priority: %d\n", currentLane->priority);
        currentLane = currentLane->next;
    } while (currentLane != intersection->head);
}

int main() {
    srand(time(NULL)); // Seed for random number generation.

    int typeInput;
    printf("Enter intersection type (0 for T, 1 for +): ");
    if (scanf("%d", &typeInput) != 1 || (typeInput != 0 && typeInput != 1)) {
        fprintf(stderr, "Invalid intersection type.\n");
        exit(EXIT_FAILURE);
    }

    IntersectionType type = (IntersectionType)typeInput;
    Intersection* intersection = createIntersection(type);

    // Simulate an ongoing traffic management system.
    while (1) {
        calculateSignalTiming(intersection);
        displaySignalTiming(intersection);
        sleep(300); // Wait for 5 minutes (300 seconds) before fetching data again.
    }

    destroyIntersection(intersection);
    return 0;
}
