/*
 * Copyright 2011 Vincent Sanders <vince@kyllikki.org>
 *
 * Licenced under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 *
 * This file is part of png23d.
 *
 * Routines to output in STL format
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "option.h"
#include "bitmap.h"
#include "mesh.h"
#include "out_stl.h"

/* binary stl output
 *
 * UINT8[80] – Header
 * UINT32 – Number of triangles
 *
 * foreach triangle
 * REAL32[3] – Normal vector
 * REAL32[3] – Vertex 1
 * REAL32[3] – Vertex 2
 * REAL32[3] – Vertex 3
 * UINT16 – Attribute byte count
 * end
 *
 */
bool output_flat_stl(bitmap *bm, int fd, options *options)
{
    struct facets *facets;
    unsigned int floop;
    uint8_t header[80];
    bool ret = true;
    uint16_t attributes = 0;

    assert(sizeof(struct facet) == 48); /* this is foul and nasty */

    facets = gen_facets(bm, options);
    if (facets == NULL) {
        fprintf(stderr,"unable to generate triangle mesh\n");
        return false;
    }

    fprintf(stderr, "cubes %d facets %d\n", facets->cubes, facets->fcount);

    memset(header, 0, 80);

    snprintf((char *)header, 80, "Binary STL generated by png23d from %s", options->infile);

    if (write(fd, header, 80) != 80) {
        ret = false;
        goto output_flat_stl_error;
    }

    if (write(fd, &facets->fcount, sizeof(uint32_t)) != sizeof(uint32_t)) {
        ret = false;
        goto output_flat_stl_error;
    }

    for (floop=0; floop < facets->fcount; floop++) {
        if (write(fd, facets->f + floop,
                  sizeof(struct facet)) != sizeof(struct facet)) {
            ret = false;
            break;
        }
        if (write(fd, &attributes,
                  sizeof(attributes)) != sizeof(attributes)) {
            ret = false;
            break;
        }
    }

output_flat_stl_error:
    free_facets(facets);

    return ret;
}

static inline void output_stl_tri(FILE *outf, const struct facet *facet)
{
    fprintf(outf,
            "  facet normal %.6f %.6f %.6f\n"
            "    outer loop\n"
            "      vertex %.6f %.6f %.6f\n"
            "      vertex %.6f %.6f %.6f\n"
            "      vertex %.6f %.6f %.6f\n"
            "    endloop\n"
            "  endfacet\n",
            facet->n.x, facet->n.y, facet->n.z,
            facet->v[0].x, facet->v[0].y, facet->v[0].z,
            facet->v[1].x, facet->v[1].y, facet->v[1].z,
            facet->v[2].x, facet->v[2].y, facet->v[2].z);
}

/* ascii stl outout */
bool output_flat_astl(bitmap *bm, int fd, options *options)
{
    struct facets *facets;
    unsigned int floop;
    FILE *outf;

    outf = fdopen(dup(fd), "w");

    facets = gen_facets(bm, options);
    if (facets == NULL) {
        fprintf(stderr,"unable to generate triangle mesh\n");
        return false;
    }

    simplify_mesh(facets);

    fprintf(stderr, "cubes %d facets %d\n", facets->cubes, facets->fcount);

    fprintf(outf, "solid png2stl_Model\n");

    for (floop = 0; floop < facets->fcount; floop++) {
        output_stl_tri(outf, facets->f + floop);
    }

    fprintf(outf, "endsolid png2stl_Model\n");

    free_facets(facets);

    fclose(outf);

    return true;
}
