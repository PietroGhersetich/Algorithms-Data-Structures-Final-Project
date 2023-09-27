#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#define COMMAND_ADD 'a'
#define COMMAND_STATION 's'
#define COMMAND_CAR 'a'
#define COMMAND_DEMOLISH_STATION 'd'
#define COMMAND_SCRAP_CAR 'r'
#define COMMAND_PLAN 'p'
#define MAX_LINE_LEN 5180
#define HEAP_LEN 512

//max-heap structure to represent cars in stations
typedef struct {
  unsigned int car_range[HEAP_LEN];
  int heap_size;
} ParkingLotMaxHeap_t;

//BST structure to represent the stations
typedef struct StationsTreeNode_ {
  unsigned int dist;
  ParkingLotMaxHeap_t lot;
  struct StationsTreeNode_ *left, *right, *p;
} StationsTreeNode_t;

typedef struct {
  StationsTreeNode_t *root;
} StationsTree_t;

//dynamic-list structure to represent the path to the destination
typedef struct node_ {
  unsigned int dist;
  struct node_ *next;
} node_t;

//directed acyclic graph structure to find path when src < dst
typedef struct dag_ {
  unsigned int dist, d, max_car;
  struct dag_ *p;
  struct dag_ *next;
} dag_t;

//dynamic-list methods
node_t* push(node_t* head, unsigned int dist) {
  node_t *tmp;
  tmp = malloc(sizeof(node_t));
  if(tmp != NULL){
    tmp->dist = dist;
    tmp->next = head;
    head = tmp;
  } else {
    printf("Out of Memory\n");
    exit(1);
  }
  return head;
}

node_t* delete_path(node_t* path) { //deletes all nodes besides the last one (dst)
  node_t* tmp;
  while(path->next != NULL){
    tmp = path;
    path = path->next;
    free(tmp);
  }
  return path;
}

node_t* delete_path_till_next_is(node_t* path, unsigned int key) { //deletes all nodes before the one with the given key (excluding the one before it)
  node_t* tmp;
  while(path->next->dist != key){
    tmp = path;
    path = path->next;
    free(tmp);
  }
  return path;
}

void print_list_and_destroy(node_t* path) {
  node_t* tmp;
  while(path->next != NULL){
    tmp = path;
    printf("%d ", path->dist);
    path = path->next;
    free(tmp);
  }
  printf("%d\n", path->dist);
  free(path);
}

//max-heap methods
int heap_parent(int i) {
  return (i-1)/2;
}

int heap_left(int i) {
  return (2*i) + 1;
}

int heap_right(int i) {
  return (2*i) + 2;
}

unsigned int heap_maximum(ParkingLotMaxHeap_t a) {
  if (a.heap_size != 0)
    return (a.car_range[0]);
  else
    return 0;
}

void max_heapify(ParkingLotMaxHeap_t *a, int i) {
  int l, r, largest;
  unsigned int tmp;
  l = heap_left(i);
  r = heap_right(i);
  if(l < a->heap_size && a->car_range[l] > a->car_range[i])
    largest = l;
  else
    largest = i;
  if(r < a->heap_size && a->car_range[r] > a->car_range[largest])
    largest = r;
  if(largest != i) {
    tmp = a->car_range[i];
    a->car_range[i] = a->car_range[largest];
    a->car_range[largest] = tmp;
    max_heapify(a, largest);
  }
}

// OSS: a->car_range must have been initialized to a given array (not yet ordered)
void build_max_heap(ParkingLotMaxHeap_t *a) {
  int i;
  for(i = (a->heap_size / 2) - 1; i >= 0; i--)
    max_heapify(a, i);
}

void heap_increase_key(ParkingLotMaxHeap_t *a, int i, unsigned int key) {
  unsigned int tmp;
  if(key < a->car_range[i]) {
    printf("Max-Heap-Increase-Key Error: new key is smaller than current key\n");
    exit(1);
  }
  a->car_range[i] = key;
  while(i > 0 && a->car_range[heap_parent(i)] < a->car_range[i]) {
    tmp = a->car_range[i];
    a->car_range[i] = a->car_range[heap_parent(i)];
    a->car_range[heap_parent(i)] = tmp;
    i = heap_parent(i);
  }
}

void max_heap_insert(ParkingLotMaxHeap_t *a, unsigned int key) {
  a->heap_size += 1;
  a->car_range[a->heap_size - 1] = 0;
  heap_increase_key(a, a->heap_size - 1, key);
}

void max_heap_delete(ParkingLotMaxHeap_t *a, int i) {
  a->heap_size -= 1;
  a->car_range[i] = a->car_range[a->heap_size];
  max_heapify(a, i);
}

//searches linearly if key is in the Max-Heap, if yes returns its position, if not -1
int max_heap_search(ParkingLotMaxHeap_t a, unsigned int key) {
  int i, found;
  for(i = 0, found = 0; i < a.heap_size && !found; i++)
    if(a.car_range[i] == key)
      found = 1;
  if(!found)
    i = 0;
  return (i - 1);
}

void print_max_heap(ParkingLotMaxHeap_t a) {
  int i;
  for (i = 0; i < a.heap_size; i++)
    printf("%d ", a.car_range[i]);
  printf("\n");
}

//BST methods

StationsTreeNode_t* bst_new_node(unsigned int key) {
  StationsTreeNode_t *tmp;
  tmp = malloc(sizeof(StationsTreeNode_t));
  if(tmp != NULL){
    tmp->dist = key;
    tmp->left = NULL;
    tmp->right = NULL;
    tmp->p = NULL;
  } else {
    printf("BST allocating node Error: Out of memory\n");
    exit(1);
  }
  return tmp;
}

StationsTreeNode_t* bst_minimum(StationsTreeNode_t *x) {
  while(x->left != NULL)
    x = x->left;
  return x;
}

StationsTreeNode_t* bst_maximum(StationsTreeNode_t *x) {
  while(x->right != NULL)
    x = x->right;
  return x;
}

StationsTreeNode_t* bst_predecessor(StationsTreeNode_t *x) {
  StationsTreeNode_t *y;
  if(x->left != NULL)
    return bst_maximum(x->left);
  y = x->p;
  //we go up the tree from x until we encounter a node that is the right child of its parent;
  while(y != NULL && x == y->left){
    x = y;
    y = y->p;
  }
  return y;
}

StationsTreeNode_t* bst_successor(StationsTreeNode_t *x) {
  StationsTreeNode_t *y;
  //leftmost node in x’s right subtree
  if(x->right != NULL)
    return bst_minimum(x->right);
  y = x->p;
  //we go up the tree from x until we encounter a node that is the left child of its parent;
  while(y != NULL && x == y->right){
    x = y;
    y = y->p;
  }
  return y;
}

//if bst_insert finds a node with the same key ad x.dist it does not add it => no duplicates
int bst_insert(StationsTree_t *t, StationsTreeNode_t *x) {
  StationsTreeNode_t *curr, *prev;
  int success;
  success = 1;
  prev = NULL;
  curr = t->root;
  while (curr != NULL && success) {
    prev = curr;
    if (x->dist < curr->dist)
      curr = curr->left;
    else if (x->dist == curr->dist)
      success = 0;
    else
      curr = curr->right;
  }
  if(success) {
    x->p = prev;
    if (prev == NULL)
      t->root = x;
    else if (x->dist < prev->dist)
      prev->left = x;
    else
      prev->right = x;
  }
  return success;
}

StationsTreeNode_t* bst_search(StationsTreeNode_t *x, unsigned int key) {
  while (x != NULL && x->dist != key) {
    if (key < x->dist)
      x = x->left;
    else
      x = x->right;
  }
  return x;
}

void bst_transplant(StationsTree_t *t, StationsTreeNode_t *u, StationsTreeNode_t *v) {
  if (u->p == NULL)
    t->root = v;
  else if (u == u->p->left)
    u->p->left = v;
  else
    u->p->right = v;
  if (v != NULL)
    v->p = u->p;
}

void bst_delete_trans(StationsTree_t *t, StationsTreeNode_t *z) {
  StationsTreeNode_t  *y;
  if (z->left == NULL)
    bst_transplant(t, z, z->right);
  else if (z->right == NULL)
    bst_transplant(t, z, z->left);
  else {
    y = bst_minimum(z->right);
    if (y->p != z){
      bst_transplant(t, y, y->right);
      y->right = z->right;
      y->right->p = y;
    }
    bst_transplant(t, z, y);
    y->left = z->left;
    y->left->p = y;
  }
}

//searches for the node identified by the key dst and, if it finds it, tries to go back to the src
//using the car with the most range between the ones in the station. If it cannot reach it, prints "nessun percorso"
void path_from_end(StationsTree_t t, unsigned int src, unsigned int dst) {
  StationsTreeNode_t *curr_bst;
  node_t *path_head, *path_curr;
  unsigned int max_range;
  int reachable;
  path_head = NULL;
  curr_bst = bst_search(t.root, dst);
  if(curr_bst == NULL){
    printf("Planning Error: no ending station found\n");
    exit(1);
  } else {
    path_head = push(path_head, curr_bst->dist);
    path_curr = path_head;

    for (curr_bst = bst_predecessor(curr_bst); curr_bst != NULL && curr_bst->dist >= src; curr_bst = bst_predecessor(curr_bst)) {
      reachable = 1;
      max_range = heap_maximum(curr_bst->lot);
      if(curr_bst->dist < src) {
        printf("Planning Error: no src station found and killed the process\n");
        exit(1);
      }

      if (curr_bst->dist + max_range >= dst) { //can reach dst from station
        path_head = delete_path(path_curr);
        path_head = push(path_head, curr_bst->dist);
        path_curr = path_head;
      } else {
        //try if I can reach one of the stations in path from the current one
        while (reachable && path_curr->next != NULL) {
          if (curr_bst->dist + max_range >= path_curr->dist)
            path_curr = path_curr->next;
          else
            reachable = 0;
        }

        if (path_curr != path_head) { //reached a station in path
          path_head = delete_path_till_next_is(path_head, path_curr->dist);
          path_head = push(path_head, curr_bst->dist);
          path_curr = path_head;
        }
      }
    }

    if (path_head->dist != src)
      printf("nessun percorso\n");
    else
      print_list_and_destroy(path_head);
  }
}

void bst_inorder_tree_walk(StationsTreeNode_t *x) {
  if(x != NULL){
    bst_inorder_tree_walk(x->left);
    printf("%d -> ", x->dist);
    bst_inorder_tree_walk(x->right);
  }
}

void bst_inorder_tree_walk_print_heap(StationsTreeNode_t *x) {
  if(x != NULL){
    bst_inorder_tree_walk_print_heap(x->left);
    printf("%d -> ", x->dist);
    print_max_heap(x->lot);
    bst_inorder_tree_walk_print_heap(x->right);
  }
}

//dag methods

dag_t* dag_push(dag_t* head, StationsTreeNode_t *curr_node) {
  dag_t *tmp;
  tmp = malloc(sizeof(dag_t));
  if(tmp != NULL){
    tmp->dist = curr_node->dist;
    tmp->d = UINT_MAX;
    tmp->p = NULL;
    tmp->max_car = heap_maximum(curr_node->lot);
    tmp->next = head;
    head = tmp;
  } else {
    printf("Out of Memory\n");
    exit(1);
  }
  return head;
}

dag_t* dag_delete_all(dag_t* dag){
  dag_t* tmp;
  while(dag != NULL){
    tmp = dag;
    dag = dag->next;
    free(tmp);
  }
  return NULL;
}

void dag_print_shortest_path_and_destroy(dag_t* dag) {
  dag_t* tmp;
  unsigned int *path;
  int i, path_len;
  path_len = (int) (dag->d + 1);
  path = malloc(path_len * sizeof(unsigned int));
  i = path_len - 1;
  while(dag != NULL && i >= 0){
    tmp = dag;
    path[i] = dag->dist;
    dag = dag->p;
    free(tmp);
    i--;
  }
  for(i = 0; i < path_len - 1; i++)
    printf("%d ", path[i]);
  printf("%d\n", path[i]);
  free(path);
}

void dag_shortest_path(StationsTree_t t, unsigned int src, unsigned int dst) { //dst < src
  StationsTreeNode_t *curr_node;
  dag_t *dag, *curr_dag, *curr_dag_adj;
  int reached;
  reached = 1;
  dag = NULL;
  for(curr_node = bst_search(t.root, dst); curr_node != NULL && curr_node->dist <= src; curr_node = bst_successor(curr_node))
    dag = dag_push(dag, curr_node);
  dag->d = 0; //initialize src d = 0
  //I obtained a topologically sorted dag
  if(dag->max_car == 0)
    reached = 0;
  for(curr_dag = dag; curr_dag->next != NULL && reached; curr_dag = curr_dag->next) {
    curr_dag_adj = curr_dag->next;
    if(curr_dag != dag && curr_dag->p == NULL)
      reached = 0;
    while (curr_dag_adj != NULL && (curr_dag->dist <= curr_dag->max_car + curr_dag_adj->dist)) { //relaxing adjacent nodes
      if (curr_dag_adj->d >= curr_dag->d + 1) {
        curr_dag_adj->d = curr_dag->d + 1;
        curr_dag_adj->p = curr_dag;
      }
      curr_dag_adj = curr_dag_adj->next;
    }
  }
  if (curr_dag->p == NULL || !reached) {
    printf("nessun percorso\n");
    dag = dag_delete_all(dag);
  } else
    dag_print_shortest_path_and_destroy(curr_dag);
}

//helper functions
unsigned int char_to_u_int(const char* str) {
  int i, pot;
  unsigned int res;
  for(i = 0; str[i] != '\0'; i++);
  for(i -= 1, res = 0, pot = 1; i >= 0 && res != -1; i--){
    if(str[i] - '0' < 0 || str[i] - '0' > 10)
      continue;
    else{
      res += pot * (str[i] - '0');
      pot *= 10;
    }
  }
  return res;
}

void copy_array(unsigned int *dst, const unsigned int *src, unsigned int dim) {
  int i;
  for(i = 0; i < dim; i++) {
    dst[i] = src[i];
  }
}

void path_from_start(StationsTree_t t, unsigned int src, unsigned int dst) {
  dag_shortest_path(t, src, dst);
}

void add_station(StationsTree_t *t, unsigned int stationDist, unsigned int numCars, unsigned int *carsToAdd) {
  int success;
  StationsTreeNode_t *curr;
  curr = bst_new_node(stationDist);
  curr->lot.heap_size = (int) numCars;
  success = bst_insert(t, curr);
  if (!success)
    printf("non aggiunta\n");
  else{
    copy_array(curr->lot.car_range, carsToAdd, numCars);
    build_max_heap(&curr->lot);
    printf("aggiunta\n");
  }
}

void demolish_station(StationsTree_t *t, unsigned int stationDist) {
  StationsTreeNode_t *x;
  x = bst_search(t->root, stationDist);
  if (x != NULL) {
    bst_delete_trans(t, x);
    printf("demolita\n");
  } else
    printf("non demolita\n");
}

void add_car(StationsTree_t *t, unsigned int stationDist, unsigned int carRange) {
  StationsTreeNode_t *x;
  x = bst_search(t->root, stationDist);
  if (x != NULL) {
    max_heap_insert(&x->lot, carRange);
    printf("aggiunta\n");
  } else
    printf("non aggiunta\n");
}

void scrap_car(StationsTree_t *t, unsigned int stationDist, unsigned int carRange) {
  StationsTreeNode_t *x;
  int i;
  x = bst_search(t->root, stationDist);
  if (x != NULL) {
    i = max_heap_search(x->lot, carRange);
    if (i != -1) {
      max_heap_delete(&x->lot, i);
      printf("rottamata\n");
    } else
      printf("non rottamata\n");
  } else
    printf("non rottamata\n");
}

void plan_path(StationsTree_t t, unsigned int startingStation, unsigned int endingStation){
  if (startingStation == endingStation) {
    printf("%d\n", startingStation);
  } else {
    if (startingStation < endingStation) {
      path_from_end(t, startingStation, endingStation);
    } else {
      path_from_start(t, startingStation, endingStation);
    }
  }
}

void print_array(unsigned int *a, int dim) {
  for(int i = 0; i < dim; i++)
    printf("%d ", a[i]);
  printf("\n");
}

int main() {
  StationsTree_t t;
  char line[MAX_LINE_LEN], *cmd;
  unsigned int numCars, stationDist, carRange, startingStation, endingStation, *carsToAdd;
  int i;

  //BST initialization
  t.root = NULL;

  while(fgets(line, MAX_LINE_LEN, stdin) != NULL) {

    cmd = strtok(line, " ");

    switch (cmd[0]) {

      case COMMAND_ADD:
        switch (cmd[9]) {
          case COMMAND_STATION:
            cmd = strtok(NULL, " ");
            stationDist = char_to_u_int(cmd);
            cmd = strtok(NULL, " ");
            numCars = char_to_u_int(cmd);
            i = 0;
            carsToAdd = malloc(numCars * sizeof(unsigned int));
            while(i < numCars){
              cmd = strtok(NULL, " ");
              carRange = char_to_u_int(cmd);
              carsToAdd[i] = carRange;
              i++;
            }
            add_station(&t, stationDist, numCars, carsToAdd);
            free(carsToAdd);
            break;

          case COMMAND_CAR:
            cmd = strtok(NULL, " ");
            stationDist = char_to_u_int(cmd);
            cmd = strtok(NULL, " ");
            carRange = char_to_u_int(cmd);
            add_car(&t, stationDist, carRange);
            break;
        }

        break;

      case COMMAND_DEMOLISH_STATION:
        cmd = strtok(NULL, " ");
        stationDist = char_to_u_int(cmd);
        demolish_station(&t, stationDist);
        break;

      case COMMAND_SCRAP_CAR:
        cmd = strtok(NULL, " ");
        stationDist = char_to_u_int(cmd);
        cmd = strtok(NULL, " ");
        carRange = char_to_u_int(cmd);
        scrap_car(&t, stationDist, carRange);
        break;

      case COMMAND_PLAN:
        cmd = strtok(NULL, " ");
        startingStation = char_to_u_int(cmd);
        cmd = strtok(NULL, " ");
        endingStation = char_to_u_int(cmd);
        plan_path(t, startingStation, endingStation);
        break;

    }
  }
  return 0;
}