#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h> // For sleep()
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
    int waitingTime;  // Time the lane has been waiting
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

typedef struct PriorityQueueNode {
    Lane* lane;
    struct PriorityQueueNode* next;
} PriorityQueueNode;

typedef struct {
    PriorityQueueNode* head;
} PriorityQueue;

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

void destroyIntersection(Intersection* intersection) {
    Lane* currentLane = intersection->head;
    do {
        Lane* temp = currentLane;
        currentLane = currentLane->next;
        free(temp);
    } while (currentLane != intersection->head);
    free(intersection);
}

void updateVehicleCounts(Intersection* intersection) {
    // Simulate real-time data fetching. Replace with actual sensor/camera data.
    Lane* currentLane = intersection->head;
    do {
        // Example: Randomly update vehicle counts for demonstration.
        currentLane->vehicleCount += rand() % 5;
        currentLane->waitingTime += 10; // Simulate waiting time increase.
        // Example: Randomly set emergency vehicle presence
        currentLane->emergencyVehicle = rand() % 2;
        currentLane = currentLane->next;
    } while (currentLane != intersection->head);
}

void inputVehicleCounts(Intersection* intersection) {
    Lane* currentLane = intersection->head;
    int i = 1;
    do {
        printf("Enter vehicle count for lane %d: ", i++);
        if (scanf("%d", &currentLane->vehicleCount) != 1 || currentLane->vehicleCount < 0) {
            fprintf(stderr, "Invalid vehicle count.\n");
            exit(EXIT_FAILURE);
        }
        printf("Is there an emergency vehicle in lane %d? (0 for No, 1 for Yes): ", i - 1);
        if (scanf("%d", &currentLane->emergencyVehicle) != 1 || (currentLane->emergencyVehicle < 0 || currentLane->emergencyVehicle > 1)) {
            fprintf(stderr, "Invalid input for emergency vehicle.\n");
            exit(EXIT_FAILURE);
        }
        currentLane = currentLane->next;
    } while (currentLane != intersection->head);
}

void calculateGreenTime(Lane* lane, int totalVehicles, int laneCount) {
    float ratio = (float)lane->vehicleCount / totalVehicles;
    lane->greenTime = (int)(ratio * BASE_GREEN_TIME * laneCount);
}

void calculateRedTime(Lane* lane, int totalTime) {
    lane->redTime = totalTime - lane->greenTime;
}

void calculatePriority(Lane* lane) {
    if (lane->emergencyVehicle) {
        lane->priority = INT_MAX; // Assign maximum priority to emergency vehicles
    } else {
        lane->priority = lane->vehicleCount; // Use vehicle count as priority for other lanes
    }
}

void calculateSignalTiming(Intersection* intersection) {
    int totalVehicles = 0;
    int totalTime = 0;

    updateVehicleCounts(intersection); // Fetch real-time vehicle counts.

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

PriorityQueue* createPriorityQueue() {
    PriorityQueue* pq = (PriorityQueue*)malloc(sizeof(PriorityQueue));
    if (!pq) {
        fprintf(stderr, "Memory allocation failed for PriorityQueue.\n");
        exit(EXIT_FAILURE);
    }
    pq->head = NULL;
    return pq;
}

void destroyPriorityQueue(PriorityQueue* pq) {
    while (pq->head) {
        PriorityQueueNode* temp = pq->head;
        pq->head = pq->head->next;
        free(temp);
    }
    free(pq);
}

void enqueue(PriorityQueue* pq, Lane* lane) {
    PriorityQueueNode* newNode = (PriorityQueueNode*)malloc(sizeof(PriorityQueueNode));
    if (!newNode) {
        fprintf(stderr, "Memory allocation failed for PriorityQueueNode.\n");
        exit(EXIT_FAILURE);
    }
    newNode->lane = lane;
    newNode->next = NULL;

    if (!pq->head || pq->head->lane->priority < lane->priority) {
        newNode->next = pq->head;
        pq->head = newNode;
    } else {
        PriorityQueueNode* current = pq->head;
        while (current->next && current->next->lane->priority >= lane->priority) {
            current = current->next;
        }
        newNode->next = current->next;
        current->next = newNode;
    }
}

void displaySignalTiming(Intersection* intersection) {
    printf("\nSignal Timing and Priority Sequence:\n");

    PriorityQueue* pq = createPriorityQueue();

    Lane* currentLane = intersection->head;
    do {
        enqueue(pq, currentLane);
        currentLane = currentLane->next;
    } while (currentLane != intersection->head);

    PriorityQueueNode* currentPQNode = pq->head;
    while (currentPQNode != NULL) {
        printf("Lane %d: Green signal for %d seconds, Red signal for %d seconds (Priority: %d, Emergency: %s)\n",
               currentPQNode->lane->laneNumber,
               currentPQNode->lane->greenTime,
               currentPQNode->lane->redTime,
               currentPQNode->lane->priority,
               currentPQNode->lane->emergencyVehicle ? "Yes" : "No");
        currentPQNode = currentPQNode->next;
    }

    printf("\nPriority Sequence: ");
    currentPQNode = pq->head;
    while (currentPQNode != NULL) {
        printf("Lane %d", currentPQNode->lane->laneNumber);
        if (currentPQNode->next) {
            printf(" > ");
        }
        currentPQNode = currentPQNode->next;
    }
    printf("\n");

    destroyPriorityQueue(pq);
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

    inputVehicleCounts(intersection);

    // Simulate an ongoing traffic management system.
    while (1) {
        calculateSignalTiming(intersection);
        displaySignalTiming(intersection);
        sleep(10); // Wait before re-evaluating the signal timings (simulation of real-time update).
    }

    destroyIntersection(intersection);
    return 0;
}
