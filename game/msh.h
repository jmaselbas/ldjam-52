/* SPDX-License-Identifier: ISC */
#ifndef MSH_H
#define MSH_H

#ifndef MSH_MAGIC
#define MSH_MAGIC "msh"
#endif

#ifndef MSH_MINALIGN
#define MSH_MINALIGN sizeof(float)
#endif

#include <stdint.h>

struct msh_head {
	char magic[4];
	uint32_t off;
	uint32_t len;
};

struct msh_buff {
	union {
		char _pak_name[56];
		struct {
			char name[56-8];
			uint8_t vsize; /* a vertex size */
			uint8_t _rsvd[3];
			uint32_t type;
		};
	};
	uint32_t off;
	uint32_t len;
};

struct msh {
	size_t size;
	size_t count;
	void *data;
	struct msh_buff *buff;
};

int msh_from_memory(unsigned int size, const void *data, struct msh *msh);
const struct msh_buff *msh_buff_by_index(const struct msh *msh, unsigned int index);
const struct msh_buff *msh_buff_by_name(const struct msh *msh, const char *name);
const void *msh_buff_data(const struct msh *msh, struct msh_buff *buf);

#ifdef MSH_WRITER
void msh_create(struct msh *m);
void msh_free(struct msh *m);
int msh_push_buff(struct msh *m, const char *name, size_t vsize, size_t nelem, const void *data);
void msh_write_file(struct msh *m, FILE *f);
#endif

#endif /* MSH_H */

#ifdef MSH_IMPLEMENTATION
#include <stdio.h>
#include <string.h>

static int msh_err(const char *msg)
{
	fprintf(stderr, "%s\n", msg);
	return -1;
}

int
msh_from_memory(unsigned int size, const void *data, struct msh *msh)
{
	const struct msh_head *hdr = data;
	struct msh m = {};

	if (size < sizeof(hdr))
		return msh_err("no header");

	if (memcmp(MSH_MAGIC, hdr->magic, sizeof(hdr->magic)) != 0)
		return msh_err("bad magic");

	if (size < (hdr->off + hdr->len))
		return msh_err("not enough data");

	m.size = size;
	m.data = (void *)data;
	m.count = hdr->len / sizeof(struct msh_buff);
	m.buff = m.data + hdr->off;
	*msh = m;

	return 0;
}

const void *
msh_buff_data(const struct msh *msh, struct msh_buff *buf)
{
	if (!buf)
		return NULL;
	if (buf->off > msh->size)
		return NULL;
	return msh->data + buf->off;
}

const struct msh_buff *
msh_buff_by_index(const struct msh *msh, unsigned int index)
{
	const struct msh_head *hdr = msh->data;

	if ((index * sizeof(struct msh_buff)) >= hdr->len)
		return NULL;
	return &msh->buff[index];
}

const struct msh_buff *
msh_buff_by_name(const struct msh *msh, const char *name)
{
	const struct msh_buff *it;
	unsigned int i;

	for (i = 0; (it = msh_buff_by_index(msh, i)); i++) {
		if (strcmp(name, it->name) == 0)
			return it;
	}

	return NULL;
}

#ifdef MSH_WRITER
void
msh_create(struct msh *m)
{
	memset(m, 0, sizeof(*m));
}

void
msh_free(struct msh *m)
{
	free(m->data);
	free(m->buff);
	memset(m, 0, sizeof(*m));
}

#define MSH_ALIGN(x, a) ((x) + ((a) - (x)) % (a))

int
msh_push_buff(struct msh *m, const char *name,
	      size_t vsize, size_t nelem, const void *data)
{
	size_t size = vsize * nelem;
	void *p;
	struct msh_buff mb = {
		.off = sizeof(struct msh_head) + m->size,
		.len = size,
	};

	size = MSH_ALIGN(size, MSH_MINALIGN);
	p = realloc(m->data, m->size + size);
	if (!p)
		return msh_err("realloc");
	m->data = p;
	p = realloc(m->buff, (m->count + 1) * sizeof(struct msh_buff));
	if (!p)
		return msh_err("realloc");
	m->buff = p;

	/* copy the buffer data */
	memcpy(m->data + m->size, data, mb.len);
	/* zero out padding bytes for alignement */
	memset(m->data + m->size + mb.len, 0, size - mb.len);
	m->size += size;

	snprintf(mb.name, sizeof(mb.name), "%s", name);
	mb.vsize = vsize;
	m->buff[m->count] = mb;
	m->count++;

	return 0;
}

void
msh_write_file(struct msh *m, FILE *f)
{
	size_t off = m->size + sizeof(struct msh_head);
	size_t len = m->count * sizeof(struct msh_buff);
	struct msh_head hdr = {
		.magic = MSH_MAGIC,
		.off = off,
		.len = len,
	};
	fwrite(&hdr, sizeof(hdr), 1, f);
	fwrite(m->data, m->size, 1, f);
	fwrite(m->buff, m->count, sizeof(struct msh_buff), f);
}

#endif /* MSH_WRITER */
#endif /* MSH_IMPLEMENTATION */
