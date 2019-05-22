#ifndef BFB_H
#define BFB_H

#define BFB_MAGIC               ("BFB!*000")
#define BFB_MAJOR               (131073)
#define BFB_MINOR               (1)
#define BFB_AUTHOR              ("BlueFang")

/*#define BFB_VTX_POSITION       (1)
#define BFB_VTX_NORMAL         (2)
#define BFB_VTX_UV0            (3)
#define BFB_VTX_UV1            (4)
#define BFB_VTX_UV2            (5)
#define BFB_VTX_UV3D           (6)
#define BFB_VTX_DIFFUSE        (7)

#define BFB_VTX_FEAT            (7)
#define BFB_GET_VTX_FEAT(v, i)  ((v.descriptor >> (pos << 2)) & BFB_VTX_FEAT)*/

#define BFB_VTX_POSITION        (0)
#define BFB_VTX_NORMAL          (1)
#define BFB_VTX_UV0             (2)
#define BFB_VTX_UV1             (3)
#define BFB_VTX_UV2             (4)
#define BFB_VTX_UV30            (5)
#define BFB_VTX_DIFFUSE         (6)

#define BFB_GET_VTX_FEAT(v, i)  ((v.descriptor >> i) & 1)
#define BFB_SET_VTX_FEAT(i)     (1 << i)

#define BFB_SPHERE              (1)
#define BFB_BOUNDINGBOX         (3)
#define BFB_CAPSULE             (4)
#define BFB_MESHDATA            (6)
#define BFB_MESH                (5)
#define BFB_SKINNED_MESH        (8)

#define BFB_ENTRY_SIZE          (76)
#define BFB_MESH_SIZE           (BFB_ENTRY_SIZE + 45)
#define BFB_MESH_DATA_SIZE      (BFB_ENTRY_SIZE + 73)
#define BFB_CAPSULE_SIZE        (BFB_ENTRY_SIZE + 28)
#define BFB_SPHERE_SIZE         (BFB_ENTRY_SIZE + 16)
#define BFB_BOUNDING_BOX_SIZE   (BFB_ENTRY_SIZE + 76)

struct bfb_file;
struct bfb_header;
struct bfb_entry;

struct bfb_capsule;
struct bfb_sphere;
struct bfb_boundingbox;

struct bfb_mesh;
struct bfb_mesh_data;

struct bfb_vertex;
struct bfb_bone;

/**
 * Index for data stored in BFB files
 */
typedef int bfb_index;

/**
 * File Header for BFB file
 */
typedef struct bfb_header {

        char    magic[8];
        int     major;
        int     minor;
        char    author[64];
        int     block_count;

} bfb_header;

/**
 * Contains all data associated
 * with a BFB file
 */
typedef struct bfb_file {

        struct  bfb_header header;
        struct  bfb_entry** entries;

} bfb_file;

/**
 * Parent of all entry types
 */
typedef struct bfb_entry {

        int     block_id;
        int     block_type;
        int     block_end;
        char    block_name[64];

} bfb_entry;

typedef struct bfb_capsule {

        struct  bfb_entry super;

        float start[3];
        float end[3];
        float radius;

} bfb_capsule;

typedef struct bfb_sphere {

        struct  bfb_entry super;

        float position[3];
        float radius;

} bfb_sphere;

typedef struct bfb_boundingbox {

        struct  bfb_entry super;

        float transform[16];
        float position[3];

} bfb_boundingbox;

/**
 * Mesh Data
 */
typedef struct bfb_mesh_data {

        struct  bfb_entry super;

        int     vertex_descriptor;
        int     vertex_stride;
        int     vertex_count;

        int     index_stride;
        int     index_count;

        struct          bfb_vertex* vertices;
        bfb_index*      indices;

} bfb_mesh_data;

/**
 * Mesh
 */
typedef struct bfb_mesh {

        struct  bfb_entry super;

        int data_id;
        int unknown0;
        int index_start;
        int index_count;
        int vertex_start;
        int vertex_count;
        int tri_count;

        float position[3];
        float scale;

} bfb_mesh;

/**
 * Describes a vertex after it is loaded into memory
 * Not all fields will be present in file
 */
typedef struct bfb_vertex {

        float position[3];
        float normal[3];
        float uv0[2];
        float uv1[2];
        float uv2[2];
        float uv3d0[3];
        float uv3d1[3];
        char diffuse[4];

} bfb_vertex;

typedef struct bfb_bone {

        int             id;
        int             parent_id;
        bfb_index       vertex_id[4];
        float           vertex_weight[4];

} bfb_bone;

#endif

