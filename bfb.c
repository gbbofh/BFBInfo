#include "bfb.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#define PROGRAM_NAME ("ldbfb")

void    bfb_error(char* fmt, ...)
{
        int rv;
        int ps;
        char* buf;
        va_list ap;

        ps = sysconf(_SC_PAGE_SIZE);
        assert(ps != -1);

        buf = malloc(ps);
        assert(buf != NULL);

        va_start(ap, fmt);
        rv = vsnprintf(buf, ps, fmt, ap);
        va_end(ap);

        perror(buf);

        free(buf);
}

void    bfb_get_header(bfb_header* head, char* buf)
{
        memcpy(head->magic, buf, 8);
        memcpy(&head->major, buf + 8, 4);
        memcpy(&head->minor, buf + 12, 4);
        memcpy(head->author, buf + 16, 64);
        memcpy(&head->block_count, buf + 80, 4);
}

void    bfb_get_entry(bfb_entry* entry, char* buf)
{
        memcpy(&entry->block_id, buf, 4);
        memcpy(&entry->block_type, buf + 4, 4);
        memcpy(&entry->block_end, buf + 8, 4);
        memcpy(entry->block_name, buf + 12, 64);

        entry->block_type &= 0xFF;

        /**
         * I have no idea where this came from, but it breaks everything --
         * what the fuck was I looking at, and/or thinking, when I wrote this
         * line of code???
         */
        //entry->block_end &= 0x0F;
}

void    bfb_get_capsule(bfb_capsule* capsule, int fd)
{
        int rv;
        int cp;
        int ps;
        char* buf;

        memset((char*)capsule + sizeof(bfb_entry), 0,
                        sizeof(bfb_capsule) - sizeof(bfb_entry));

        cp = lseek(fd, 0, SEEK_CUR);

        ps = sysconf(_SC_PAGE_SIZE);
        buf = malloc(ps);

        rv = read(fd, buf, BFB_CAPSULE_SIZE - BFB_ENTRY_SIZE);
        if(rv == -1) {

                bfb_error("%s", PROGRAM_NAME);
                abort();
        }

        memcpy(capsule->start, buf, 3 * sizeof(float));
        memcpy(capsule->end, buf + 3 * sizeof(float), 3 * sizeof(float));
        memcpy(&capsule->radius, buf + 6 * sizeof(float), sizeof(float));

        lseek(fd, cp, SEEK_SET);
        free(buf);
}

void    bfb_get_sphere(bfb_sphere* sphere, int fd)
{
        int rv;
        int cp;
        int ps;
        char* buf;

        memset((char*)sphere + sizeof(bfb_entry), 0,
                        sizeof(bfb_sphere) - sizeof(bfb_entry));

        cp = lseek(fd, 0, SEEK_CUR);

        ps = sysconf(_SC_PAGE_SIZE);
        buf = malloc(ps);

        rv = read(fd, buf, BFB_SPHERE_SIZE - BFB_ENTRY_SIZE);
        if(rv == -1) {

                bfb_error("%s", PROGRAM_NAME);
                abort();
        }

        memcpy(sphere->position, buf, 3 * sizeof(float));
        memcpy(&sphere->radius, buf, sizeof(float));

        lseek(fd, cp, SEEK_SET);
        free(buf);
}

void    bfb_get_boundingbox(bfb_boundingbox* box, int fd)
{
        int rv;
        int cp;
        int ps;
        char* buf;

        memset((char*)box + sizeof(bfb_entry), 0,
                        sizeof(bfb_boundingbox) - sizeof(bfb_entry));

        cp = lseek(fd, 0, SEEK_CUR);

        ps = sysconf(_SC_PAGE_SIZE);
        buf = malloc(ps);

        rv = read(fd, buf, BFB_BOUNDING_BOX_SIZE - BFB_ENTRY_SIZE);
        if(rv == -1) {

                bfb_error("%s", PROGRAM_NAME);
                abort();
        }

        memcpy(box->transform, buf, 16 * sizeof(float));
        memcpy(box->position, buf + 16 * sizeof(float), 3 * sizeof(float)); 

        lseek(fd, cp, SEEK_SET);
        free(buf);
}

/**
 * TODO:
 * Need to add support for T31, and T3D0
 * This method? is causing a segfault in bfb_get_mesh
 */
void    bfb_get_mesh_data(bfb_mesh_data* meshdata, int fd)
{
        int rv;
        int cp;
        int ps;
        char* buf;

        int vtx_bytes;
        int inx_bytes;
        int vtx_chunk_size;
        int inx_chunk_size;

        int vtx_offset;
        int inx_offset;

        memset((char*)meshdata + sizeof(bfb_entry), 0,
                        sizeof(bfb_mesh_data) - sizeof(bfb_entry));

        ps = sysconf(_SC_PAGE_SIZE);
        buf = malloc(ps);

        cp = lseek(fd, 0, SEEK_CUR);

        rv = read(fd, buf, BFB_MESH_DATA_SIZE - BFB_ENTRY_SIZE);
        if(rv == -1) {

                bfb_error("%s", PROGRAM_NAME);
                abort();
        }

        meshdata->vertex_descriptor |= strstr(buf, "P") ?
                                        (1 << BFB_VTX_POSITION) : 0;
        meshdata->vertex_descriptor |= strstr(buf, "N") ?
                                        (1 << BFB_VTX_NORMAL) : 0;
        meshdata->vertex_descriptor |= strstr(buf, "T0") ?
                                        (1 << BFB_VTX_UV0) : 0;
        meshdata->vertex_descriptor |= strstr(buf, "T1") ?
                                        (1 << BFB_VTX_UV1) : 0;
        meshdata->vertex_descriptor |= strstr(buf, "T2") ?
                                        (1 << BFB_VTX_UV2) : 0;
        meshdata->vertex_descriptor |= strstr(buf, "T30") ?
                                        (1 << BFB_VTX_UV30) : 0;
        meshdata->vertex_descriptor |= strstr(buf, "ND") ?
                                        (1 << BFB_VTX_DIFFUSE) : 0;

        memcpy(&meshdata->vertex_stride, buf + 65, 4);
        memcpy(&meshdata->vertex_count, buf + 69, 4);

        meshdata->vertices = malloc(meshdata->vertex_count *
                                        sizeof(bfb_vertex));

        vtx_chunk_size = ps / meshdata->vertex_stride * meshdata->vertex_stride;
        vtx_bytes = meshdata->vertex_stride * meshdata->vertex_count;
        vtx_offset = 0;

        /**
         * READ AND PROCESS VERTEX DATA
         */
        while(vtx_bytes - vtx_chunk_size >= 0) {

                rv = read(fd, buf, vtx_chunk_size);
                if(rv == -1) {

                        bfb_error("%s", PROGRAM_NAME);
                        abort();
                }

                vtx_bytes -= vtx_chunk_size;

                for(int i = 0; i < vtx_chunk_size;
                                i += meshdata->vertex_stride) {

                        int po;
                        bfb_vertex* v = (meshdata->vertices +
                                        (i + vtx_offset) /
                                        meshdata->vertex_stride);

                        po = 0;

                        if(meshdata->vertex_descriptor &
                                        (1 << BFB_VTX_POSITION)) {

                                v->position[0] = *(float*)(buf + i);
                                v->position[1] = *(float*)(buf + i + 4);
                                v->position[2] = *(float*)(buf + i + 8);

                                po += 12;
                        }

                        if(meshdata->vertex_descriptor &
                                        (1 << BFB_VTX_NORMAL)) {

                                v->normal[0] = *(float*)(buf + i + po);
                                v->normal[1] = *(float*)(buf + i + po + 4);
                                v->normal[2] = *(float*)(buf + i + po + 8);

                                po += 12;
                        }

                        if(meshdata->vertex_descriptor &
                                        (1 << BFB_VTX_DIFFUSE)) {

                                v->diffuse[0] = *(char*)(buf + i + po);
                                v->diffuse[1] = *(char*)(buf + i + po + 1);
                                v->diffuse[2] = *(char*)(buf + i + po + 2);
                                v->diffuse[3] = *(char*)(buf + i + po + 3);

                                po += 4;
                        }

                        if(meshdata->vertex_descriptor &
                                        (1 << BFB_VTX_UV0)) {

                                v->uv0[0] = *(float*)(buf + i + po);
                                v->uv0[1] = *(float*)(buf + i + po + 4);

                                po += 8;
                        }

                        if(meshdata->vertex_descriptor &
                                        (1 << BFB_VTX_UV1)) {

                                v->uv1[0] = *(float*)(buf + i + po);
                                v->uv1[1] = *(float*)(buf + i + po + 4);

                                po += 8;
                        }

                        if(meshdata->vertex_descriptor &
                                        (1 << BFB_VTX_UV2)) {

                                v->uv2[0] = *(float*)(buf + i + po);
                                v->uv2[1] = *(float*)(buf + i + po + 4);

                                po += 8;
                        }

                        if(meshdata->vertex_descriptor &
                                        (1 << BFB_VTX_UV30)) {

                                v->uv3d0[0] = *(float*)(buf + i + po);
                                v->uv3d0[1] = *(float*)(buf + i + po + 4);
                                v->uv3d0[2] = *(float*)(buf + i + po + 8);

                                po += 12;
                        }
                }

                vtx_offset += vtx_chunk_size;
        }

        if(vtx_bytes > 0) {

                rv = read(fd, buf, vtx_bytes);
                if(rv == -1) {

                        bfb_error("%s", PROGRAM_NAME);
                        abort();
                }

                for(int i = 0; i < vtx_bytes;
                                i += meshdata->vertex_stride) {

                        int po;
                        bfb_vertex* v = (meshdata->vertices +
                                        (i + vtx_offset) /
                                        meshdata->vertex_stride);

                        po = 0;

                        if(meshdata->vertex_descriptor &
                                        (1 << BFB_VTX_POSITION)) {

                                v->position[0] = *(float*)(buf + i);
                                v->position[1] = *(float*)(buf + i + 4);
                                v->position[2] = *(float*)(buf + i + 8);

                                po += 12;
                        }

                        if(meshdata->vertex_descriptor &
                                        (1 << BFB_VTX_NORMAL)) {

                                v->normal[0] = *(float*)(buf + i + po);
                                v->normal[1] = *(float*)(buf + i + po + 4);
                                v->normal[2] = *(float*)(buf + i + po + 8);

                                po += 12;
                        }

                        if(meshdata->vertex_descriptor &
                                        (1 << BFB_VTX_DIFFUSE)) {

                                v->diffuse[0] = *(char*)(buf + i + po);
                                v->diffuse[1] = *(char*)(buf + i + po + 1);
                                v->diffuse[2] = *(char*)(buf + i + po + 2);
                                v->diffuse[3] = *(char*)(buf + i + po + 3);

                                po += 4;
                        }

                        if(meshdata->vertex_descriptor &
                                        (1 << BFB_VTX_UV0)) {

                                v->uv0[0] = *(float*)(buf + i + po);
                                v->uv0[1] = *(float*)(buf + i + po + 4);

                                po += 8;
                        }

                        if(meshdata->vertex_descriptor &
                                        (1 << BFB_VTX_UV1)) {

                                v->uv1[0] = *(float*)(buf + i + po);
                                v->uv1[1] = *(float*)(buf + i + po + 4);

                                po += 8;
                        }

                        if(meshdata->vertex_descriptor &
                                        (1 << BFB_VTX_UV2)) {

                                v->uv2[0] = *(float*)(buf + i + po);
                                v->uv2[1] = *(float*)(buf + i + po + 4);

                                po += 8;
                        }

                        if(meshdata->vertex_descriptor &
                                        (1 << BFB_VTX_UV30)) {

                                v->uv3d0[0] = *(float*)(buf + i + po);
                                v->uv3d0[1] = *(float*)(buf + i + po + 4);
                                v->uv3d0[2] = *(float*)(buf + i + po + 8);

                                po += 12;
                        }
                }

                vtx_offset += vtx_bytes;
                vtx_bytes = 0;
        }

        /**
         * READ AND PROCESS INDEX DATA
         */
        rv = read(fd, buf, 5);
        if(rv == -1) {

                bfb_error("%s", PROGRAM_NAME);
                abort();
        }

        memcpy(&meshdata->index_stride, buf, 1);
        memcpy(&meshdata->index_count, buf + 1, 2);

        meshdata->indices = malloc(meshdata->index_count *
                                        sizeof(bfb_index));

        inx_chunk_size = ps / meshdata->index_stride * meshdata->index_stride;
        inx_bytes = meshdata->index_stride * meshdata->index_count;
        inx_offset = 0;

        while(inx_bytes - inx_chunk_size >= 0) {

                rv = read(fd, buf, inx_chunk_size);
                if(rv == -1) {

                        bfb_error("%s", PROGRAM_NAME);
                        abort();
                }

                for(int i = 0; i < inx_chunk_size;
                                i += meshdata->index_stride) {

                        *(meshdata->indices + (inx_offset + i) /
                                meshdata->index_stride) = *(short*)(buf + i);
                }

                inx_offset += inx_chunk_size;
                inx_bytes -= inx_chunk_size;
        }

        if(inx_bytes > 0) {

                rv = read(fd, buf, inx_bytes);
                if(rv == -1) {

                        bfb_error("%s", PROGRAM_NAME);
                        abort();
                }

                for(int i = 0; i < inx_bytes;
                                i += meshdata->index_stride) {

                        *(meshdata->indices + (inx_offset + i) /
                                meshdata->index_stride) = *(short*)(buf + i);
                }

                inx_offset += inx_bytes;
                inx_bytes = 0;
        }

        lseek(fd, cp, SEEK_SET);
        free(buf);
}

/**
 * TODO:
 * Add support for reading remaining mesh data
 */
void    bfb_get_mesh(bfb_mesh* mesh, int fd)
{
        int rv;
        int cp;
        int ps;
        char* buf;

        int vtx_bytes;
        int vtx_chunk_size;

        int vtx_offset;

        memset((char*)mesh + sizeof(bfb_entry), 0,
                        sizeof(bfb_mesh) - sizeof(bfb_entry));

        ps = sysconf(_SC_PAGE_SIZE);
        buf = malloc(ps);

        cp = lseek(fd, 0, SEEK_CUR);

        rv = read(fd, buf, BFB_MESH_SIZE - BFB_ENTRY_SIZE);
        if(rv == -1) {

                bfb_error("%s", PROGRAM_NAME);
                abort();
        }

        mesh->data_id = *(int*)(buf + 1);
        mesh->unknown0 = *(int*)(buf + 5);
        mesh->index_start = *(int*)(buf + 9);
        mesh->index_count = *(int*)(buf + 13);
        mesh->vertex_start = *(int*)(buf + 17);
        mesh->vertex_count = *(int*)(buf + 21);
        mesh->tri_count = *(int*)(buf + 25);

        memcpy(mesh->position, buf + 29, 3 * sizeof(float));
        memcpy(&mesh->scale, buf + 29 + 3 * sizeof(float), sizeof(float));

        vtx_bytes = 0;
        vtx_chunk_size = 0;
        vtx_offset = 0;

        /*while(vtx_bytes - vtx_chunk_size >= 0) {

                rv = read(fd, buf, vtx_chunk_size);
                if(rv == -1) {

                        bfb_error("%s", PROGRAM_NAME);
                        abort();
                }

                vtx_bytes -= vtx_chunk_size;
        }*/

        lseek(fd, cp, SEEK_SET);
        free(buf);
}

void    bfb_read_file(bfb_file* fp, char* path)
{
        int i;
        int rv;
        int fd;
        int ps;
        int sp;
        char* buf;

        sp = 0;
        ps = sysconf(_SC_PAGE_SIZE);
        if(ps == -1) {

                bfb_error("%s", PROGRAM_NAME);
                abort();
        }

        buf = malloc(ps);
        assert(buf != NULL);

        fd = open(path, O_RDONLY);
        if(fd == -1) {

                bfb_error("%s", PROGRAM_NAME);
                abort();
        }

        rv = read(fd, buf, 88);
        if(rv == -1) {

                bfb_error("%s", PROGRAM_NAME);
                abort();
        }

        bfb_get_header(&fp->header, buf);
        sp = rv;

        fp->entries = malloc(fp->header.block_count * sizeof(bfb_entry*));

        for(i = 0; i < fp->header.block_count; i++) {

                bfb_entry e;

                rv = lseek(fd, sp, SEEK_SET);
                if(rv == -1) {

                        bfb_error("%s", PROGRAM_NAME);
                        abort();
                }

                rv = read(fd, buf, BFB_ENTRY_SIZE);
                if(rv == -1) {

                        bfb_error("%s", PROGRAM_NAME);
                        abort();
                }

                bfb_get_entry(&e, buf);

                switch(e.block_type) {

                        case BFB_SPHERE:
                                fp->entries[i] = malloc(sizeof(bfb_sphere));
                                memcpy(fp->entries[i], &e, sizeof(bfb_entry));
                                bfb_get_sphere((bfb_sphere*)fp->entries[i], fd);
                                break;
                        case BFB_BOUNDINGBOX:
                                fp->entries[i] = malloc(sizeof(bfb_boundingbox));
                                memcpy(fp->entries[i], &e, sizeof(bfb_entry));
                                bfb_get_boundingbox((bfb_boundingbox*)fp->entries[i], fd);
                                break;
                        case BFB_CAPSULE:
                                fp->entries[i] = malloc(sizeof(bfb_capsule));
                                memcpy(fp->entries[i], &e, sizeof(bfb_entry));
                                bfb_get_capsule((bfb_capsule*)fp->entries[i], fd);
                                break;
                        case BFB_MESHDATA:
                                fp->entries[i] = malloc(sizeof(bfb_mesh_data));
                                memcpy(fp->entries[i], &e, sizeof(bfb_entry));
                                bfb_get_mesh_data((bfb_mesh_data*)fp->entries[i], fd);
                                break;
                        case BFB_MESH:
                        case BFB_SKINNED_MESH:
                                fp->entries[i] = malloc(sizeof(bfb_mesh));
                                memcpy(fp->entries[i], &e, sizeof(bfb_entry));
                                bfb_get_mesh((bfb_mesh*)fp->entries[i], fd);
                                break;
                        default:
                                printf("%s: unknown block type.\n", PROGRAM_NAME);
                                fp->entries[i] = malloc(sizeof(bfb_entry));
                                memcpy(fp->entries[i], &e, sizeof(bfb_entry));
                                break;
                }

                sp = e.block_end;
        }

        close(fd);
        free(buf);
}

void bfb_print_header(bfb_file* file)
{
        printf("magic:       %.8s\n", file->header.magic);
        printf("author:      %s\n", file->header.author);
        printf("block count: %d\n\n", file->header.block_count);
}

void bfb_print_entry(bfb_entry* entry)
{
        printf("block id:   %x\n", entry->block_id);
        printf("block type: %x\n", entry->block_type);
        printf("block name: %s\n", entry->block_name);
        printf("block end:  %x\n\n", entry->block_end);
}

void bfb_print_mesh_data(bfb_mesh_data* data)
{
        int vd;
        int off;
        char format[64];

        memset(format, 0, 64);

        vd = data->vertex_descriptor;
        off = snprintf(format, 64, "BFRVertex");

        if(vd & (1 << BFB_VTX_POSITION)) {

                off += snprintf(format + off, 64 - off, "P");
        }
        if(vd & (1 << BFB_VTX_NORMAL)) {

                off += snprintf(format + off, 64 - off, "N");
        }
        if(vd & (1 << BFB_VTX_DIFFUSE)) {

                off += snprintf(format + off, 64 - off, "D");
        }
        if(vd & (1 << BFB_VTX_UV0)) {

                off += snprintf(format + off, 64 - off, "T0");
        }
        if(vd & (1 << BFB_VTX_UV1)) {

                off += snprintf(format + off, 64 - off, "T1");
        }
        if(vd & (1 << BFB_VTX_UV2)) {

                off += snprintf(format + off, 64 - off, "T2");
        }
        if(vd & (1 << BFB_VTX_UV30)) {

                off += snprintf(format + off, 64 - off, "T30");
        }

        printf("vertex format: %s\n", format);
        printf("vertex stride: %d\n", data->vertex_stride);
        printf("vertex count:  %d\n\n", data->vertex_count);

        printf("index stride:  %d\n", data->index_stride);
        printf("index count:   %d\n\n", data->index_count);
}

void bfb_print_mesh(bfb_mesh* mesh)
{
}

void bfb_print_capsule(bfb_capsule* capsule)
{
}

void bfb_print_sphere(bfb_sphere* sphere)
{
}

void bfb_print_boundingbox(bfb_boundingbox* box)
{
}

int main(int argc, char* argv[])
{
        int i;
        bfb_file bfb;

        if(argc < 2) {

                printf("usage: ./bfb [-e] FILE\n");
                exit(1);
        }

        bfb_read_file(&bfb, argv[1]);
        bfb_print_header(&bfb);

        for(i = 0; i < bfb.header.block_count; i++) {

                bfb_print_entry(bfb.entries[i]);

                if(bfb.entries[i]->block_type == BFB_MESHDATA) {

                        bfb_mesh_data* md;

                        md = (bfb_mesh_data*)bfb.entries[i];

                        bfb_print_mesh_data(md);

                        free(md->vertices);
                        free(md->indices);
                }
                printf("\n");
                free(bfb.entries[i]);
        }
        free(bfb.entries);
}

