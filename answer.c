#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <malloc.h>
#include <stdint.h>
#include <unistd.h>


#define QUEUE_SIZE	16
#define NODE_SIZE	0x1000


#define INCREASE_QUEUE_INDEX(INDEX, SIZE)		\
	(INDEX == (SIZE - 1)) ? 0 : INDEX + 1;
#define GET_NUM_OF_OCCUPIED_NODE(HEAD, TAIL, SIZE)	\
	(HEAD >= TAIL) ? HEAD - TAIL : HEAD + SIZE - TAIL;
#define GET_NUM_OF_EMPTY_NODE(HEAD, TAIL, SIZE)		\
	(TAIL >  HEAD) ? TAIL - HEAD : TAIL + SIZE - HEAD;


//TODO Define global data structures to be used
typedef struct {
	char *buffer;
	int bufferSizeInBytes;
} node_type;


node_type data_queue[QUEUE_SIZE];
int queue_head = 0;
int queue_tail = 0;
pthread_mutex_t head_lock;
pthread_mutex_t tail_lock;


int get_external_data(char *buffer, int bufferSizeInBytes)
{
	int data_size;

	data_size = rand() % (bufferSizeInBytes * 150 / 100);

	if (bufferSizeInBytes < data_size)
		return -1;

	memset(buffer, 0xA5, data_size);

	return data_size;
}

void process_data(char *buffer, int bufferSizeInBytes)
{
	memset(buffer, 0x0F, bufferSizeInBytes);
}


void push_data(char *buffer, int bufferSizeInBytes)
{
	int num_of_empty_node;

	pthread_mutex_lock(&head_lock);

	num_of_empty_node = GET_NUM_OF_EMPTY_NODE(queue_head, queue_tail, QUEUE_SIZE);
	while (num_of_empty_node == 0) {
		sleep(1);
		num_of_empty_node = GET_NUM_OF_EMPTY_NODE(queue_head, queue_tail, QUEUE_SIZE);
	}

	data_queue[queue_head].buffer = buffer;
	data_queue[queue_head].bufferSizeInBytes = bufferSizeInBytes;

	printf("%s : 0x%X -- 0x%X\n", __FUNCTION__, (int)(uintptr_t)buffer, bufferSizeInBytes);

	queue_head = INCREASE_QUEUE_INDEX(queue_head, QUEUE_SIZE);

	pthread_mutex_unlock(&head_lock);
}

void pop_data(char **buffer, int *bufferSizeInBytes)
{
	int num_of_occupied_node;

	pthread_mutex_lock(&tail_lock);

	num_of_occupied_node = GET_NUM_OF_OCCUPIED_NODE(queue_head, queue_tail, QUEUE_SIZE);
	while (num_of_occupied_node == 0) {
		sleep(1);
		num_of_occupied_node = GET_NUM_OF_OCCUPIED_NODE(queue_head, queue_tail, QUEUE_SIZE);
	}

	*buffer = data_queue[queue_tail].buffer;
	*bufferSizeInBytes = data_queue[queue_tail].bufferSizeInBytes;

	printf("%s : 0x%X -- 0x%X\n", __FUNCTION__, (int)(uintptr_t)*buffer, *bufferSizeInBytes);

	queue_tail = INCREASE_QUEUE_INDEX(queue_tail, QUEUE_SIZE);

	pthread_mutex_unlock(&tail_lock);
}


/**
 * This thread is responsible for pulling data off of the shared data 
 * area and processing it using the process_data() API.
 */
void *reader_thread(void *arg) {
	//TODO: Define set-up required
	char *buffer;
	int data_size;

	while (1) {
		//TODO: Define data extraction (queue) and processing 
		pop_data(&buffer, &data_size);
		process_data(buffer, data_size);
		
		free(buffer);
	}
	
	return NULL;
}


/**
 * This thread is responsible for pulling data from a device using
 * the get_external_data() API and placing it into a shared area
 * for later processing by one of the reader threads.
 */
void *writer_thread(void *arg) {
	//TODO: Define set-up required
	char *buffer;
	int data_size;
	
	while (1) {
		//TODO: Define data extraction (device) and storage
		buffer = (char *)malloc(NODE_SIZE);

		data_size = get_external_data(buffer, NODE_SIZE);
		if (data_size < 0) {
			free(buffer);
			continue;
		}

		push_data(buffer, data_size);
	}
	
	return NULL;
}


#define N 20
#define M 10
int main(int argc, char **argv) {
	int i;
	pthread_t thread_t_reader[N];
	pthread_t thread_t_writer[M];
	int status;

	pthread_mutex_init(&head_lock, NULL);
	pthread_mutex_init(&tail_lock, NULL);
	
	for (i = 0; i < N; i++) { 
		if (pthread_create(&thread_t_reader[i], NULL, reader_thread, NULL) < 0)
		{
			perror("thread create error:");
			exit(0);
		}
	}
	for (i = 0; i < M; i++) { 
		if (pthread_create(&thread_t_writer[i], NULL, writer_thread, NULL) < 0)
		{
			perror("thread create error:");
			exit(0);
		}
	}

	for (i = 0; i < N; i++) { 
		pthread_join(thread_t_reader[i], (void **)&status);
	}
	for (i = 0; i < M; i++) { 
		pthread_join(thread_t_writer[i], (void **)&status);
	}

	return 0;	
}
