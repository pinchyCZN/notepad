#include "precompiledHeaders.h"
#include <string.h>
#ifdef _DEBUG
#include <stdio.h>
#define STBI_ZERROR(x) fprintf(stderr, "%s:%d: error: %s\n", __FILE__, __LINE__, x), 0
#else
#define STBI_ZERROR(x) 0
#endif

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;
#define inline __inline

// fast-way is faster to check than jpeg huffman, but slow way is slower
#define STBI__ZFAST_BITS  9 // accelerate all cases in default tables
#define STBI__ZFAST_MASK  ((1 << STBI__ZFAST_BITS) - 1)

// zlib-style huffman encoding
// (jpegs packs from left, zlib from right, so can't share code)
struct stbi__zhuffman {
   uint16_t fast[1 << STBI__ZFAST_BITS];
   uint16_t firstcode[16];
   int maxcode[17];
   uint16_t firstsymbol[16];
   uint8_t  size[288];
   uint16_t value[288];
};

static inline int stbi__bitreverse16(int n)
{
  n = ((n & 0xAAAA) >>  1) | ((n & 0x5555) << 1);
  n = ((n & 0xCCCC) >>  2) | ((n & 0x3333) << 2);
  n = ((n & 0xF0F0) >>  4) | ((n & 0x0F0F) << 4);
  n = ((n & 0xFF00) >>  8) | ((n & 0x00FF) << 8);
  return n;
}

static inline int stbi__bit_reverse(int v, int bits)
{
   // to bit reverse n bits, reverse 16 and shift
   // e.g. 11 bits, bit reverse and shift away 5
   return stbi__bitreverse16(v) >> (16-bits);
}

static int stbi__zbuild_huffman(struct stbi__zhuffman *z, uint8_t *sizelist, int num)
{
   int i,k=0;
   int code, next_code[16], sizes[17];

   // DEFLATE spec for generating codes
   memset(sizes, 0, sizeof(sizes));
   memset(z->fast, 0, sizeof(z->fast));
   for (i=0; i < num; ++i)
      ++sizes[sizelist[i]];
   sizes[0] = 0;
   for (i=1; i < 16; ++i)
      if (sizes[i] > (1 << i))
         return STBI_ZERROR("bad sizes");
   code = 0;
   for (i=1; i < 16; ++i) {
      next_code[i] = code;
      z->firstcode[i] = (uint16_t) code;
      z->firstsymbol[i] = (uint16_t) k;
      code = (code + sizes[i]);
      if (sizes[i])
         if (code-1 >= (1 << i)) return STBI_ZERROR("bad codelengths");
      z->maxcode[i] = code << (16-i); // preshift for inner loop
      code <<= 1;
      k += sizes[i];
   }
   z->maxcode[16] = 0x10000; // sentinel
   for (i=0; i < num; ++i) {
      int s = sizelist[i];
      if (s) {
         int c = next_code[s] - z->firstcode[s] + z->firstsymbol[s];
         uint16_t fastv = (uint16_t) ((s << 9) | i);
         z->size [c] = (uint8_t     ) s;
         z->value[c] = (uint16_t) i;
         if (s <= STBI__ZFAST_BITS) {
            int j = stbi__bit_reverse(next_code[s],s);
            while (j < (1 << STBI__ZFAST_BITS)) {
               z->fast[j] = fastv;
               j += (1 << s);
            }
         }
         ++next_code[s];
      }
   }
   return 1;
}

// zlib-from-memory implementation for PNG reading
//    because PNG allows splitting the zlib stream arbitrarily,
//    and it's annoying structurally to have PNG call ZLIB call PNG,
//    we require PNG read all the IDATs and combine them into a single
//    memory buffer

typedef struct stbi__stream
{
    const uint8_t *start_in;
    const uint8_t *next_in;
    const uint8_t *end_in;

    uint8_t *start_out;
    uint8_t *next_out;
    uint8_t *end_out;

    void *cookie_in;
    void *cookie_out;

    size_t total_in;
    size_t total_out;

    int (*refill)(struct stbi__stream *);
    int (*flush)(struct stbi__stream *);

   int num_bits;
   uint32_t code_buffer;

   struct stbi__zhuffman z_length, z_distance;
} stbi__zbuf;

int refill_zeros(struct stbi__stream *stream) {
    static const uint8_t zeros[64] = {0};
    stream->start_in = stream->next_in = zeros;
    stream->end_in = zeros + sizeof(zeros);
    return 0;
}

int refill_stdio(struct stbi__stream *stream) {
    size_t n = fread((void *)stream->start_in, 1, stream->end_in - stream->start_in, (FILE *)stream->cookie_in);
    if (n) {
        stream->next_in = stream->start_in;
        stream->end_in = stream->start_in + n;
        return 0;
    }
    return refill_zeros(stream);
}

int flush_stdio(struct stbi__stream *stream) {
    size_t n = fwrite(stream->start_out, 1, stream->next_out - stream->start_out, (FILE *)stream->cookie_out);
    if (n) {
        stream->next_out = stream->start_out;
        return 0;
    }
    return -1;
}

static inline uint8_t stbi__zget8(struct stbi__stream *stream)
{
    if (stream->next_in == stream->end_in)
        stream->refill(stream);
    return *stream->next_in++;
}

static void stbi__fill_bits(stbi__zbuf *z)
{
   do {
      z->code_buffer |= (unsigned int) stbi__zget8(z) << z->num_bits;
      z->num_bits += 8;
   } while (z->num_bits <= 24);
}

static inline unsigned int stbi__zreceive(stbi__zbuf *z, int n)
{
   unsigned int k;
   if (z->num_bits < n) stbi__fill_bits(z);
   k = z->code_buffer & ((1 << n) - 1);
   z->code_buffer >>= n;
   z->num_bits -= n;
   return k;
}

static int stbi__zhuffman_decode_slowpath(stbi__zbuf *a, struct stbi__zhuffman *z)
{
   int b,s,k;
   // not resolved by fast table, so compute it the slow way
   // use jpeg approach, which requires MSbits at top
   k = stbi__bit_reverse(a->code_buffer, 16);
   for (s=STBI__ZFAST_BITS+1; ; ++s)
      if (k < z->maxcode[s])
         break;
   if (s == 16) return -1; // invalid code!
   // code size is s, so:
   b = (k >> (16-s)) - z->firstcode[s] + z->firstsymbol[s];
   a->code_buffer >>= s;
   a->num_bits -= s;
   return z->value[b];
}

static inline int stbi__zhuffman_decode(stbi__zbuf *a, struct stbi__zhuffman *z)
{
   int b,s;
   if (a->num_bits < 16) stbi__fill_bits(a);
   b = z->fast[a->code_buffer & STBI__ZFAST_MASK];
   if (b) {
      s = b >> 9;
      a->code_buffer >>= s;
      a->num_bits -= s;
      return b & 511;
   }
   return stbi__zhuffman_decode_slowpath(a, z);
}

static int stbi__zlength_base[31] = {
   3,4,5,6,7,8,9,10,11,13,
   15,17,19,23,27,31,35,43,51,59,
   67,83,99,115,131,163,195,227,258,0,0 };

static int stbi__zlength_extra[31]=
{ 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0,0,0 };

static int stbi__zdist_base[32] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,
257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577,0,0};

static int stbi__zdist_extra[32] =
{ 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};

static int stbi__parse_huffman_block(stbi__zbuf *a)
{
   uint8_t *zout = a->next_out;
   for(;;) {
      int z = stbi__zhuffman_decode(a, &a->z_length);
      if (z < 256) {
         if (z < 0) return STBI_ZERROR("bad huffman code"); // error in huffman codes
         if (zout == a->end_out) {
             a->next_out = zout;
             if (a->flush(a))
                 return 0;
             zout = a->next_out;
         }
         *zout++ = (char) z;
         //a->window[a->total_out++ % (1 << 15)] = (char)z;
         //if (a->total_out % (1 << 15) == 0)
         //    fwrite(a->window, sizeof(a->window), 1, stdout);
      } else {
         //uint8_t *p;
         int len,dist;
         if (z == 256) {
            a->next_out = zout;
            return 1;
         }
         z -= 257;
         len = stbi__zlength_base[z];
         if (stbi__zlength_extra[z]) len += stbi__zreceive(a, stbi__zlength_extra[z]);
         z = stbi__zhuffman_decode(a, &a->z_distance);
         if (z < 0) return STBI_ZERROR("bad huffman code");
         dist = stbi__zdist_base[z];
         if (stbi__zdist_extra[z]) dist += stbi__zreceive(a, stbi__zdist_extra[z]);
         //if (zout - a->zout_start < dist) return STBI_ZERROR("bad dist");
         //if (zout + len > a->zout_end) {
         //   if (!stbi__zexpand(a, zout, len)) return 0;
         //   zout = a->zout;
         //}
         //p = (uint8_t *) (zout - dist);
         //if (dist == 1) { // run of one byte; common in images.
         //   uint8_t v = *p;
         //   if (len) { do *zout++ = v; while (--len); }
         //} else {
         //   if (len) { do *zout++ = *p++; while (--len); }
         //}
         if (len) {
            uint8_t *src = zout - dist;
            if (src < a->start_out)
                src += a->end_out - a->start_out; 
            do {
                if (src == a->end_out)
                    src = a->start_out;
                if (zout == a->end_out){
                    a->next_out = zout;
                    a->flush(a);
                    zout = a->next_out;
                }
                *zout++ = *src++;
            } while (--len);

            ////zout += len;
            //size_t x = a->total_out - dist;
            //do {
            //    a->window[a->total_out++ % (1 << 15)] = a->window[x++ % (1 << 15)];
            //    //a->window[a->total_out % (1 << 15)] = a->window[(a->total_out - dist) % (1 << 15)];
            //    if (a->total_out % (1 << 15) == 0)
            //        fwrite(a->window, sizeof(a->window), 1, stdout);
            //} while (--len);
         }
      }
   }
}

static int stbi__compute_huffman_codes(stbi__zbuf *a)
{
   static uint8_t length_dezigzag[19] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
   struct stbi__zhuffman z_codelength;
   uint8_t lencodes[286+32+137];//padding for maximum single op
   uint8_t codelength_sizes[19];
   int i,n;

   int hlit  = stbi__zreceive(a,5) + 257;
   int hdist = stbi__zreceive(a,5) + 1;
   int hclen = stbi__zreceive(a,4) + 4;

   memset(codelength_sizes, 0, sizeof(codelength_sizes));
   for (i=0; i < hclen; ++i) {
      int s = stbi__zreceive(a,3);
      codelength_sizes[length_dezigzag[i]] = (uint8_t) s;
   }
   if (!stbi__zbuild_huffman(&z_codelength, codelength_sizes, 19)) return 0;

   n = 0;
   while (n < hlit + hdist) {
      int c = stbi__zhuffman_decode(a, &z_codelength);
      if (c < 0 || c >= 19) return STBI_ZERROR("bad codelengths");
      if (c < 16)
         lencodes[n++] = (uint8_t) c;
      else if (c == 16) {
         c = stbi__zreceive(a,2)+3;
         memset(lencodes+n, lencodes[n-1], c);
         n += c;
      } else if (c == 17) {
         c = stbi__zreceive(a,3)+3;
         memset(lencodes+n, 0, c);
         n += c;
      } else {
         c = stbi__zreceive(a,7)+11;
         memset(lencodes+n, 0, c);
         n += c;
      }
   }
   if (n != hlit+hdist) return STBI_ZERROR("bad codelengths");
   if (!stbi__zbuild_huffman(&a->z_length, lencodes, hlit)) return 0;
   if (!stbi__zbuild_huffman(&a->z_distance, lencodes+hlit, hdist)) return 0;
   return 1;
}

static int stbi__parse_uncompressed_block(stbi__zbuf *a)
{
   uint8_t header[4];
   int len,nlen,k;
   if (a->num_bits & 7)
      stbi__zreceive(a, a->num_bits & 7); // discard
   // drain the bit-packed data into header
   k = 0;
   while (a->num_bits > 0) {
      header[k++] = (uint8_t) (a->code_buffer & 255); // suppress MSVC run-time check
      a->code_buffer >>= 8;
      a->num_bits -= 8;
   }
   // now fill header the normal way
   while (k < 4)
      header[k++] = stbi__zget8(a);
   len  = header[1] * 256 + header[0];
   nlen = header[3] * 256 + header[2];
   if (nlen != (len ^ 0xffff)) return STBI_ZERROR("zlib corrupt");
#if 1
   do {
       size_t avail_out = a->end_out - a->next_out;
       while (avail_out-- && len--)
           *a->next_out++ = stbi__zget8(a);
       if (len > 0)
           a->flush(a);
   } while (len > 0);

   //while (len--) {
   //    a->window[a->total_out++ % (1 << 15)] = stbi__zget8(a);
   //    if (a->total_out % (1 << 15) == 0)
   //        fwrite(a->window, sizeof(a->window), 1, stdout);
   //}
#else
   memcpy(a->zout, a->zbuffer, len);
   a->zbuffer += len;
   a->zout += len;
#endif
   return 1;
}

// @TODO: should statically initialize these for optimal thread safety
static uint8_t stbi__zdefault_length[288], stbi__zdefault_distance[32];
static void stbi__init_zdefaults(void)
{
   int i;   // use <= to match clearly with spec
   for (i=0; i <= 143; ++i)     stbi__zdefault_length[i]   = 8;
   for (   ; i <= 255; ++i)     stbi__zdefault_length[i]   = 9;
   for (   ; i <= 279; ++i)     stbi__zdefault_length[i]   = 7;
   for (   ; i <= 287; ++i)     stbi__zdefault_length[i]   = 8;

   for (i=0; i <=  31; ++i)     stbi__zdefault_distance[i] = 5;
}

int stb_inflate(struct stbi__stream *a)
{
   int final, type;
   a->num_bits = 0;
   a->code_buffer = 0;
   a->total_out = 0;
   do {
      final = stbi__zreceive(a,1);
      type = stbi__zreceive(a,2);
      if (type == 0) {
         if (!stbi__parse_uncompressed_block(a)) return 0;
      } else if (type == 3) {
         return 0;
      } else {
         if (type == 1) {
            // use fixed code lengths
            if (!stbi__zdefault_distance[31]) stbi__init_zdefaults();
            if (!stbi__zbuild_huffman(&a->z_length  , stbi__zdefault_length  , 288)) return 0;
            if (!stbi__zbuild_huffman(&a->z_distance, stbi__zdefault_distance,  32)) return 0;
         } else {
            if (!stbi__compute_huffman_codes(a)) return 0;
         }
         if (!stbi__parse_huffman_block(a)) return 0;
      }
   } while (!final);
   a->flush(a);
   return 1;
}


/*-------------------------------------------------------------------------------*/

struct zip_entry {
    uint64_t uncompressed_size;
    uint64_t compressed_size;
    uint64_t local_header_offset;
    const char *filename;
    time_t mtime;
};

#pragma pack(push,1)
struct local_file_header {
    uint32_t signature;
    uint16_t version_needed;
    uint16_t flags;
    uint16_t compression_method;
    uint16_t last_mod_file_time;
    uint16_t last_mod_file_date;
    uint32_t crc_32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t file_name_length;
    uint16_t extra_field_length;
};

struct central_dir_header {
    uint32_t signature;
    uint16_t version;
    uint16_t version_needed;
    uint16_t flags;
    uint16_t compression_method;
    uint16_t last_mod_file_time;
    uint16_t last_mod_file_date;
    uint32_t crc_32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t file_name_length;
    uint16_t extra_field_length;
    uint16_t file_comment_length;
    uint16_t disk_number_start;
    uint16_t internal_file_attributes;
    uint32_t external_file_attributes;
    uint32_t local_header_offset;
};

struct end_of_central_dir_record64 {
    uint32_t signature;
    uint64_t eocdr_size;
    uint16_t version;
    uint16_t version_needed;
    uint32_t disk_number;
    uint32_t cdr_disk_number;
    uint64_t disk_num_entries;
    uint64_t num_entries;
    uint64_t cdr_size;
    uint64_t cdr_offset;
};

struct end_of_central_dir_locator64 {
    uint32_t signature;
    uint32_t eocdr_disk;
    uint64_t eocdr_offset;
    uint32_t num_disks;
};

struct end_of_central_dir_record {
    uint32_t signature;
    uint16_t disk_number;
    uint16_t cdr_disk_number;
    uint16_t disk_num_entries;
    uint16_t num_entries;
    uint32_t cdr_size;
    uint32_t cdr_offset;
    uint16_t ZIP_file_comment_length;
};
#pragma pack(pop,1)

static size_t zip_read(struct zip_entry **ptr, FILE *stream) {
    // find the end of central directory record
    uint32_t signature;
    long offset;
#	define UINT16_MAX 0xFFFF
#	define UINT32_MAX 0xFFFFFFFF
    struct end_of_central_dir_record eocdr;
    struct end_of_central_dir_record64 eocdr64;
	int zip64;
	struct zip_entry *entries;
	char *strings;
	size_t i,i_end;

    for (offset = sizeof(struct end_of_central_dir_record);; ++offset) {
        if (offset > UINT16_MAX || fseek(stream, -offset, SEEK_END) || !fread(&signature, sizeof(signature), 1, stream))
            return 0;
        if (signature == 0x06054B50)
            break;
    }

    // read end of central directory record
    if (!(fseek(stream, -offset, SEEK_END) == 0 &&
          fread(&eocdr, sizeof(eocdr), 1, stream) &&
          eocdr.signature == 0x06054B50 &&
          eocdr.disk_number == 0 &&
          eocdr.cdr_disk_number == 0 &&
          eocdr.disk_num_entries == eocdr.num_entries))
        return 0;

    // check for zip64
    zip64 = eocdr.num_entries == UINT16_MAX || eocdr.cdr_offset == UINT32_MAX || eocdr.cdr_size == UINT32_MAX;
    if (zip64) {
        // zip64 end of central directory locator
        struct end_of_central_dir_locator64 eocdl64;
        if (!(fseek(stream, -offset - sizeof(eocdl64), SEEK_END) == 0 &&
              fread(&eocdl64, sizeof(eocdl64), 1, stream) &&
              eocdl64.signature == 0x07064B50 &&
              eocdl64.eocdr_disk == 0 &&
              eocdl64.num_disks == 1))
            return 0;
        // zip64 end of central directory record
        if (!(fseek(stream, (long)eocdl64.eocdr_offset, SEEK_SET) == 0 &&
              fread(&eocdr64, sizeof(eocdr64), 1, stream) &&
              eocdr64.signature == 0x06064B50 &&
              eocdr64.disk_number == 0 &&
              eocdr64.cdr_disk_number == 0 &&
              eocdr64.disk_num_entries == eocdr64.num_entries))
            return 0;
    }

    // seek to central directory record
    if (fseek(stream, zip64 ? (long)eocdr64.cdr_offset : (long)eocdr.cdr_offset, SEEK_SET))
        return 0;

    // alloc buffer for entries array and filenames
    entries = (struct zip_entry *)malloc(zip64 ? (size_t)eocdr64.cdr_size : (size_t)eocdr.cdr_size);
    if (!entries)
        return 0;

    // store filenames after entries array
    strings = (char *)(entries + (zip64 ? (size_t)eocdr64.num_entries : (size_t)eocdr.num_entries));

    for (i = 0, i_end = zip64 ? (size_t)eocdr64.num_entries : (size_t)eocdr.num_entries; i < i_end; ++i) {
        // read central directory header, filename, extra field and skip comment
        struct central_dir_header cdh;
		struct zip_entry *entry;
		char *extra;
        if (!(fread(&cdh, sizeof(cdh), 1, stream) &&
              cdh.signature == 0x02014B50 &&
              fread(strings, cdh.file_name_length + cdh.extra_field_length, 1, stream) &&
              fseek(stream, cdh.file_comment_length, SEEK_CUR) == 0)) {
            free(entries);
            return 0;
        }

        entry = entries + i;
        entry->uncompressed_size = cdh.uncompressed_size;
        entry->compressed_size = cdh.compressed_size;
        entry->local_header_offset = cdh.local_header_offset;

        // find zip64 extended information extra field
        for (extra = strings + cdh.file_name_length; extra != strings + cdh.file_name_length + cdh.extra_field_length;) {
            uint16_t header_id;
            uint16_t data_size;
            memcpy(&header_id, extra, sizeof(header_id));
            extra += sizeof(header_id);

            memcpy(&data_size, extra, sizeof(data_size));
            extra += sizeof(data_size);

            switch (header_id) {
            case 0x0001:
                if (cdh.uncompressed_size == UINT32_MAX) {
                    memcpy(&entry->uncompressed_size, extra, sizeof(entry->uncompressed_size));
                    extra += sizeof(entry->uncompressed_size);
                }
                if (cdh.compressed_size == UINT32_MAX) {
                    memcpy(&entry->compressed_size, extra, sizeof(entry->compressed_size));
                    extra += sizeof(entry->compressed_size);
                }
                if (cdh.local_header_offset == UINT32_MAX) {
                    memcpy(&entry->local_header_offset, extra, sizeof(entry->local_header_offset));
                    extra += sizeof(entry->local_header_offset);
                }
                if (cdh.disk_number_start == UINT16_MAX) {
                    extra += sizeof(uint32_t);
                }
                break;
            default:
                extra += data_size;
                break;
            }
        }

        entry->filename = strings;
        strings += cdh.file_name_length;
        *strings++ = '\0';
		{
			struct tm t={0};
            t.tm_sec = (cdh.last_mod_file_time << 1) & 0x3F,
            t.tm_min = (cdh.last_mod_file_time >> 5) & 0x3F,
            t.tm_hour = (cdh.last_mod_file_time >> 11) & 0x1F,
            t.tm_mday = cdh.last_mod_file_date & 0x1F,
            t.tm_mon = ((cdh.last_mod_file_date >> 5) & 0xF) - 1,
            t.tm_year = ((cdh.last_mod_file_date >> 9) & 0x7F) + 1980 - 1900,
            t.tm_isdst = -1,

        entry->mtime = mktime(&t);
		}
    }

    *ptr = entries;
    return zip64 ? (int)eocdr64.num_entries : (int)eocdr.num_entries;
}

static int zip_seek(FILE *stream, const struct zip_entry *entry) {
    struct local_file_header lfh;
    return !(fseek(stream, (long)entry->local_header_offset, SEEK_SET) == 0 &&
             fread(&lfh, sizeof(lfh), 1, stream) &&
             lfh.signature == 0x04034B50 &&
             fseek(stream, lfh.file_name_length + lfh.extra_field_length, SEEK_CUR) == 0);
}

static int zip_store(FILE *stream, const char *filename, const void *data, size_t size) {
	time_t t;
	struct tm *tm;
	struct local_file_header lfh;
    long offset = ftell(stream);
    if (offset == -1)
        return 1;

    t = time(NULL);
    tm = localtime(&t);

    
        lfh.signature = 0x04034B50,
        lfh.version_needed = 10,
        lfh.flags = 0,
        lfh.compression_method = 0,
        lfh.last_mod_file_time = tm->tm_hour << 11 | tm->tm_min << 5 | tm->tm_sec >> 1,
        lfh.last_mod_file_date = (tm->tm_year - 80) << 9 | (tm->tm_mon + 1) << 5 | tm->tm_mday,
        lfh.crc_32 = 0,
        lfh.compressed_size = size,
        lfh.uncompressed_size = size,
        lfh.file_name_length = strlen(filename),
        lfh.extra_field_length = 0;

    fwrite(&lfh, sizeof(lfh), 1, stream);
    fwrite(filename, lfh.file_name_length, 1, stream);
    fwrite(data, size, 1, stream);

    return 0;
}

static int zip_finalize(FILE *stream) {
    long offset = 0;
    struct local_file_header lfh;
    char filename[1024];
	struct end_of_central_dir_record eocdr;

    
        eocdr.signature = 0x06054B50,
        eocdr.disk_number = 0,
        eocdr.cdr_disk_number = 0,
        eocdr.disk_num_entries = 0,
        eocdr.num_entries = 0,
        eocdr.cdr_size = 0,
        eocdr.cdr_offset = ftell(stream),
        eocdr.ZIP_file_comment_length = 0;

    while (fseek(stream, offset, SEEK_SET) == 0 &&
           fread(&lfh, sizeof(lfh), 1, stream) &&
           lfh.signature == 0x04034B50 &&
           lfh.file_name_length < sizeof(filename) &&
           fread(filename, lfh.file_name_length, 1, stream)) {
		struct central_dir_header cdh;
        printf("F %.*s\n", lfh.file_name_length, filename);
        
            cdh.signature = 0x02014B50,
            cdh.version = 10,
            cdh.version_needed = lfh.version_needed,
            cdh.flags = lfh.flags,
            cdh.compression_method = lfh.compression_method,
            cdh.last_mod_file_time = lfh.last_mod_file_time,
            cdh.last_mod_file_date = lfh.last_mod_file_date,
            cdh.crc_32 = lfh.crc_32,
            cdh.compressed_size = lfh.compressed_size,
            cdh.uncompressed_size = lfh.uncompressed_size,
            cdh.file_name_length = lfh.file_name_length,
            cdh.extra_field_length = 0,
            cdh.file_comment_length = 0,
            cdh.disk_number_start = 0,
            cdh.internal_file_attributes = 0,
            cdh.external_file_attributes = 0,
            cdh.local_header_offset = offset;
        
        fseek(stream, 0, SEEK_END);
        fwrite(&cdh, sizeof(cdh), 1, stream);
        fwrite(filename, lfh.file_name_length, 1, stream);

        ++eocdr.num_entries;
        ++eocdr.disk_num_entries;

        offset += sizeof(lfh) + lfh.file_name_length + lfh.compressed_size;
    }

    fseek(stream, 0, SEEK_END);
    eocdr.cdr_size = ftell(stream) - eocdr.cdr_offset;
    fwrite(&eocdr, sizeof(eocdr), 1, stream);

    return 0;
}


