#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>




#define NUM_PRODUCTS 20
#define NUM_CLIENTS 10
#define MAX_REQUESTS 1 // 1 etima gia kathe pelati




// ta proioda
typedef struct {
    char description[30];
    float price;
    int item_count;
} Product;

Product catalog[NUM_PRODUCTS];
int total_requests = 0, successful_requests = 0, failed_requests = 0;
float total_revenue = 0.0;


// arxikopiisi tou katalogou
void initialize_catalog() {
    for (int i = 0; i < NUM_PRODUCTS; i++) {
        snprintf(catalog[i].description, 30, "Product %d", i + 1);
        catalog[i].price = (i + 1) * 10.0; // timi proiodos
        catalog[i].item_count = 2;        // 2 texamia
    }
}


// pelatis
void customer(int client_id, int to_eshop[2], int from_eshop[2]) {
    close(to_eshop[0]);
    close(from_eshop[1]);

    srand(time(NULL) + client_id); // tuxea noumera

    for (int i = 0; i < MAX_REQUESTS; i++) {
        int product_id = rand() % NUM_PRODUCTS; // tuxeo proion 
        write(to_eshop[1], &product_id, sizeof(int));

        char response[100];
        read(from_eshop[0], response, sizeof(response));
        printf("Client %d: %s\n", client_id, response);

        sleep(1); // 1 sec kathisterisi
    }

    close(to_eshop[1]);
    close(from_eshop[0]);
    exit(0);
}



// litourgua eshop
void eshop(int client_pipes[NUM_CLIENTS][2], int client_responses[NUM_CLIENTS][2]) {
    close(client_pipes[0][1]);
    close(client_responses[0][0]);

    for (int i = 0; i < NUM_CLIENTS; i++) {
        close(client_pipes[i][1]);
        close(client_responses[i][0]);
    }

    int requested_products = 0, sold_products = 0;

    // eksipiretisi pelaton
    for (int i = 0; i < NUM_CLIENTS; i++) {
        for (int j = 0; j < MAX_REQUESTS; j++) {
            int product_id;
            read(client_pipes[i][0], &product_id, sizeof(int));
            requested_products++;

            char response[100];
            if (catalog[product_id].item_count > 0) {
                catalog[product_id].item_count--; // meiosi tou apothematos pou uparxei
                float cost = catalog[product_id].price;
                snprintf(response, sizeof(response), "Purchase complete, your total is: %.2f euro", cost);

                total_revenue += cost;
                successful_requests++;
                sold_products++;
            } else {
                snprintf(response, sizeof(response), "Products unavailable, request failed");
                failed_requests++;
            }

            write(client_responses[i][1], response, sizeof(response));
            sleep(1); // perimeno gia apotelesmata
        }
    }

    printf("\n%d requests were made, where %d succeeded and %d failed\n", 
        requested_products, successful_requests, failed_requests);
    printf("%d products were requested, where %d products were bought, totaling %.2f euros\n", 
        requested_products, sold_products, total_revenue);

    for (int i = 0; i < NUM_CLIENTS; i++) {
        close(client_pipes[i][0]);
        close(client_responses[i][1]);
    }
}



// to main program
int main() {
    initialize_catalog();

    int client_pipes[NUM_CLIENTS][2];
    int client_responses[NUM_CLIENTS][2];

    // dimiourgia ton solinon tou kathe pelati
    for (int i = 0; i < NUM_CLIENTS; i++) {
        pipe(client_pipes[i]);
        pipe(client_responses[i]);
    }


    // dimiourgia pelaton me fork
    for (int i = 0; i < NUM_CLIENTS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // kleisimo ton solinon pou den xriazode apo ton pelati
            for (int j = 0; j < NUM_CLIENTS; j++) {
                if (j != i) {
                    close(client_pipes[j][0]);
                    close(client_pipes[j][1]);
                    close(client_responses[j][0]);
                    close(client_responses[j][1]);
                }
            }
            customer(i, client_pipes[i], client_responses[i]);
        }
    }


    // ektelesi eshop
    eshop(client_pipes, client_responses);


    // perimeno gia to kleisimo ton pelaton
    for (int i = 0; i < NUM_CLIENTS; i++) {
        wait(NULL);
    }

    return 0;
}
