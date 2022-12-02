#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h> // sqrtf
#include <limits.h> // INT_MAX

/*****************************************************************
 * Ladici makra. Vypnout jejich efekt lze definici makra
 * NDEBUG, napr.:
 *   a) pri prekladu argumentem prekladaci -DNDEBUG
 *   b) v souboru (na radek pred #include <assert.h>
 *      #define NDEBUG
 */
#ifdef NDEBUG
#define debug(s)
#define dfmt(s, ...)
#define dint(i)
#define dfloat(f)
#else

// vypise ladici retezec
#define debug(s) printf("- %s\n", s)

// vypise formatovany ladici vystup - pouziti podobne jako printf
#define dfmt(s, ...) printf(" - "__FILE__":%u: "s"\n",__LINE__,__VA_ARGS__)

// vypise ladici informaci o promenne - pouziti dint(identifikator_promenne)
#define dint(i) printf(" - " __FILE__ ":%u: " #i " = %d\n", __LINE__, i)

// vypise ladici informaci o promenne typu float - pouziti
// dfloat(identifikator_promenne)
#define dfloat(f) printf(" - " __FILE__ ":%u: " #f " = %g\n", __LINE__, f)

#endif

/*****************************************************************
 * Deklarace potrebnych datovych typu:
 *
 * TYTO DEKLARACE NEMENTE
 *
 *   struct obj_t - struktura objektu: identifikator a souradnice
 *   struct cluster_t - shluk objektu:
 *      pocet objektu ve shluku,
 *      kapacita shluku (pocet objektu, pro ktere je rezervovano
 *          misto v poli),
 *      ukazatel na pole shluku.
 */

struct obj_t {
    int id;
    float x;
    float y;
};

struct cluster_t {
    int size;
    int capacity;
    struct obj_t *obj;
};

/// Not to change
struct cluster_t *resize_cluster(struct cluster_t *c, int new_cap);
void sort_cluster(struct cluster_t *c);
static int obj_sort_compar(const void *a, const void *b);
void print_cluster(struct cluster_t *c);
void print_clusters(struct cluster_t *carr, int narr);
void init_cluster(struct cluster_t *c, int cap);
void clear_cluster(struct cluster_t *c);
void append_cluster(struct cluster_t *c, struct obj_t obj);
void merge_clusters(struct cluster_t *c1, struct cluster_t *c2);
int remove_cluster(struct cluster_t *carr, int narr, int idx);
float obj_distance(struct obj_t *o1, struct obj_t *o2);
float cluster_distance(struct cluster_t *c1, struct cluster_t *c2);
void find_neighbours(struct cluster_t *carr, int narr, int *first, int *second);
int load_clusters(char *filename, struct cluster_t **arr);

int main(int argc, char *argv[])
{
    struct cluster_t *clusters;
    char *fileName = argv[1];
    int number;
    sscanf(argv[2], "%d", &number);

    int count = load_clusters(fileName, &clusters);

    int first = 0, second = 0;
    while(count != number){
        find_neighbours(clusters, count, &first, &second);
        merge_clusters(&clusters[first], &clusters[second]);
        count = remove_cluster(clusters, count, second);
    }

    print_clusters(clusters, count);

    for (int i = 0; i < count; ++i) {
        clear_cluster(&clusters[i]);
    }

    free(clusters);

    return 0;
}

int load_clusters(char *filename, struct cluster_t **arr) {
    assert(arr != NULL);
    // ToDo: check input

    FILE *file = fopen(filename, "r");
    int count = 0, id = 0;
    float x = 0, y = 0;

    fscanf(file, "count=%d", &count);
    *arr = (struct cluster_t *) malloc(sizeof(struct cluster_t) * count);

    for (int i = 0; i < count; ++i) {
        fscanf(file, "%d %f %f\n", &id, &x, &y);

        struct cluster_t *cluster = &*(*arr+i);
        init_cluster(cluster, 1);
        cluster->size = 1;
        cluster->obj->id = id;
        cluster->obj->x = x;
        cluster->obj->y = y;
    }
    fclose(file);

    return count;
}

void init_cluster(struct cluster_t *c, int cap) {
    assert(c != NULL);
    assert(cap >= 0);

    c->size = 0;
    c->capacity = cap;
    c->obj = (cap == 0)?NULL:(struct obj_t *) malloc(sizeof(struct obj_t) * cap);
}

void clear_cluster(struct cluster_t *c) {
    free(c->obj);
    init_cluster(c, 0);
}

const int CLUSTER_CHUNK = 10;

struct cluster_t *resize_cluster(struct cluster_t *c, int new_cap)
{
    // TUTO FUNKCI NEMENTE
    assert(c);
    assert(c->capacity >= 0);
    assert(new_cap >= 0);

    if (c->capacity >= new_cap)
        return c;

    size_t size = sizeof(struct obj_t) * new_cap;

    void *arr = realloc(c->obj, size);
    if (arr == NULL)
        return NULL;

    c->obj = (struct obj_t*)arr;
    c->capacity = new_cap;
    return c;
}

void append_cluster(struct cluster_t *c, struct obj_t obj)
{
    if(c->size == c->capacity) {
        resize_cluster(c, c->capacity + 1);
    }

    c->obj[(c->size)++] = obj;
}

void merge_clusters(struct cluster_t *c1, struct cluster_t *c2)
{
    assert(c1 != NULL);
    assert(c2 != NULL);

    for (int i = 0; i < c2->size; ++i) {
        append_cluster(c1, c2->obj[i]);
    }
    sort_cluster(c1);

}

int remove_cluster(struct cluster_t *carr, int narr, int idx)
{
    assert(idx < narr);
    assert(narr > 0);

    clear_cluster(&carr[idx]);
    carr[idx] = carr[narr-1];
    init_cluster(&carr[narr-1], 0);


    return narr - 1;
}

float obj_distance(struct obj_t *o1, struct obj_t *o2) {
    assert(o1 != NULL);
    assert(o2 != NULL);

    return sqrtf(powf((o2->x - o1->x), 2) + powf((o2->y - o1->y), 2));
}

float cluster_distance(struct cluster_t *c1, struct cluster_t *c2)
{
    assert(c1 != NULL);
    assert(c1->size > 0);
    assert(c2 != NULL);
    assert(c2->size > 0);

    float biggest_dist = 0, current_dist;
    for(int i = 0; i < c1->size; i++) {
        for(int j = 0; j < c2->size; j++) {
            current_dist = obj_distance(&c1->obj[i], &c2->obj[j]);
            if(current_dist > biggest_dist){
                biggest_dist = current_dist;
            }
        }
    }

    return biggest_dist;
}

void find_neighbours(struct cluster_t *carr, int narr, int *first, int *second) {
    assert(narr > 0);

    float smallest_distance = 1000, current_distance = 0;
    for(int i = 0; i < narr - 1; ++i) {
        for(int j = i + 1; j < narr; ++j) {
            current_distance = cluster_distance(&carr[i], &carr[j]);
            if(current_distance < smallest_distance){
                smallest_distance = current_distance;
                *first = i;
                *second = j;
            }
        }
    }
    //printf("F:%d S:%d ", *first, *second);
}

static int obj_sort_compar(const void *a, const void *b)
{
    // TUTO FUNKCI NEMENTE
    const struct obj_t *o1 = (const struct obj_t *)a;
    const struct obj_t *o2 = (const struct obj_t *)b;
    if (o1->id < o2->id) return -1;
    if (o1->id > o2->id) return 1;
    return 0;
}

void sort_cluster(struct cluster_t *c)
{
    // TUTO FUNKCI NEMENTE
    qsort(c->obj, c->size, sizeof(struct obj_t), &obj_sort_compar);
}

void print_cluster(struct cluster_t *c)
{
    // TUTO FUNKCI NEMENTE
    for (int i = 0; i < c->size; i++)
    {
        if (i) putchar(' ');
        printf("%d[%g,%g]", c->obj[i].id, c->obj[i].x, c->obj[i].y);
    }
    putchar('\n');
}

void print_clusters(struct cluster_t *carr, int narr) {
    printf("Clusters:\n");
    for (int i = 0; i < narr; i++)
    {
        printf("cluster %d: ", i);
        print_cluster(&carr[i]);
    }
}
