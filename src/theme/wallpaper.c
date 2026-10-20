/* Fondos de pantalla generados con la paleta del tema (PNG, sin librerías externas).
 * Estilos: aurora, ondas, degradado, circulos, petalos, dunas, malla, cristal, topografico y seda
 * (liso = sin imagen, solo el color de fondo). */
#include "theme.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "rros_profile.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ---- PNG: deflate con Huffman fijo y LZ77 ---- */

struct out {
	uint8_t *p;
	size_t n, cap;
	uint32_t bits;
	int nbits;
	int err;
};

static void put_byte(struct out *o, uint8_t b)
{
	if (o->n == o->cap) {
		size_t cap = o->cap ? o->cap * 2 : 1 << 16;
		uint8_t *np = realloc(o->p, cap);

		if (!np) {
			o->err = 1;
			return;
		}
		o->p = np;
		o->cap = cap;
	}
	o->p[o->n++] = b;
}

/* bits menos significativos primero (orden de deflate) */
static void put_bits(struct out *o, uint32_t v, int n)
{
	o->bits |= v << o->nbits;
	o->nbits += n;
	while (o->nbits >= 8) {
		put_byte(o, (uint8_t)(o->bits & 255));
		o->bits >>= 8;
		o->nbits -= 8;
	}
}

/* los códigos de Huffman se escriben empezando por su bit más significativo */
static void put_code(struct out *o, uint32_t code, int len)
{
	uint32_t r = 0;

	for (int i = 0; i < len; i++)
		r |= ((code >> i) & 1) << (len - 1 - i);
	put_bits(o, r, len);
}

static void put_litlen(struct out *o, int v)
{
	if (v < 144)
		put_code(o, 0x30 + (uint32_t)v, 8);
	else if (v < 256)
		put_code(o, 0x190 + (uint32_t)(v - 144), 9);
	else if (v < 280)
		put_code(o, (uint32_t)(v - 256), 7);
	else
		put_code(o, 0xc0 + (uint32_t)(v - 280), 8);
}

static const int len_base[29] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59,
				  67, 83, 99, 115, 131, 163, 195, 227, 258 };
static const int len_extra[29] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
static const int dist_base[30] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769,
				   1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 };
static const int dist_extra[30] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10,
				    11, 11, 12, 12, 13, 13 };

static void put_match(struct out *o, int len, int dist)
{
	int i = 28, d = 29;

	while (len_base[i] > len)
		i--;
	put_litlen(o, 257 + i);
	put_bits(o, (uint32_t)(len - len_base[i]), len_extra[i]);
	while (dist_base[d] > dist)
		d--;
	put_code(o, (uint32_t)d, 5);
	put_bits(o, (uint32_t)(dist - dist_base[d]), dist_extra[d]);
}

#define WIN 32768
#define HBITS 15

static void deflate_fixed(struct out *o, const uint8_t *d, size_t n)
{
	int32_t *head = malloc(sizeof(int32_t) << HBITS), *prev = malloc(sizeof(int32_t) * WIN);
	size_t i = 0;

	if (!head || !prev) {
		o->err = 1;
		free(head);
		free(prev);
		return;
	}
	for (size_t k = 0; k < (1u << HBITS); k++)
		head[k] = -1;
	put_bits(o, 1, 1);   /* último bloque */
	put_bits(o, 1, 2);   /* Huffman fijo */
	while (i < n) {
		int best = 0, bdist = 0;

		if (i + 2 < n) {
			uint32_t h = (((uint32_t)d[i] << 10) ^ ((uint32_t)d[i + 1] << 5) ^ d[i + 2]) & ((1u << HBITS) - 1);
			int32_t cand = head[h];
			int chain = 12;

			while (cand >= 0 && i - (size_t)cand <= WIN && chain--) {
				size_t max = n - i < 258 ? n - i : 258, l = 0;

				while (l < max && d[(size_t)cand + l] == d[i + l])
					l++;
				if ((int)l > best) {
					best = (int)l;
					bdist = (int)(i - (size_t)cand);
					if (l == max)
						break;
				}
				cand = prev[cand & (WIN - 1)];
			}
			prev[i & (WIN - 1)] = head[h];
			head[h] = (int32_t)i;
		}
		if (best >= 3) {
			put_match(o, best, bdist);
			/* las posiciones dentro de la coincidencia también entran en el diccionario */
			for (size_t k = i + 1; k < i + (size_t)best && k + 2 < n; k++) {
				uint32_t h = (((uint32_t)d[k] << 10) ^ ((uint32_t)d[k + 1] << 5) ^ d[k + 2]) & ((1u << HBITS) - 1);

				prev[k & (WIN - 1)] = head[h];
				head[h] = (int32_t)k;
			}
			i += (size_t)best;
		} else {
			put_litlen(o, d[i]);
			i++;
		}
	}
	put_litlen(o, 256);
	if (o->nbits)
		put_bits(o, 0, 8 - o->nbits);
	free(head);
	free(prev);
}

static uint32_t crc_table[256];

static uint32_t crc32_of(const uint8_t *p, size_t n, uint32_t c)
{
	if (!crc_table[1])
		for (uint32_t i = 0; i < 256; i++) {
			uint32_t v = i;

			for (int k = 0; k < 8; k++)
				v = v & 1 ? 0xedb88320u ^ (v >> 1) : v >> 1;
			crc_table[i] = v;
		}
	c ^= 0xffffffffu;
	for (size_t i = 0; i < n; i++)
		c = crc_table[(c ^ p[i]) & 255] ^ (c >> 8);
	return c ^ 0xffffffffu;
}

static void be32(uint8_t *p, uint32_t v)
{
	p[0] = (uint8_t)(v >> 24);
	p[1] = (uint8_t)(v >> 16);
	p[2] = (uint8_t)(v >> 8);
	p[3] = (uint8_t)v;
}

static int chunk(FILE *f, const char *type, const uint8_t *data, size_t n)
{
	uint8_t hdr[8], crc[4];
	uint32_t c;

	be32(hdr, (uint32_t)n);
	memcpy(hdr + 4, type, 4);
	c = crc32_of((const uint8_t *)type, 4, 0);
	c = crc32_of(data, n, c);
	be32(crc, c);
	return fwrite(hdr, 1, 8, f) != 8 || (n && fwrite(data, 1, n, f) != n) || fwrite(crc, 1, 4, f) != 4;
}

/* filtros PNG por fila: el que deja los valores más pequeños (heurística habitual) */
static void filter_rows(const uint8_t *img, int w, int h, uint8_t *dst)
{
	size_t stride = (size_t)w * 3;
	uint8_t *tmp = dst;

	for (int y = 0; y < h; y++) {
		const uint8_t *row = img + (size_t)y * stride, *up = y ? row - stride : NULL;
		long best = -1;
		int bf = 0;

		for (int ft = 1; ft < 5; ft += ft == 2 ? 2 : 1) {   /* Sub, Up y Paeth */
			long sum = 0;

			for (size_t x = 0; x < stride; x++) {
				int a = x >= 3 ? row[x - 3] : 0, b = up ? up[x] : 0, c = x >= 3 && up ? up[x - 3] : 0, v;

				switch (ft) {
				case 0: v = row[x]; break;
				case 1: v = row[x] - a; break;
				case 2: v = row[x] - b; break;
				case 3: v = row[x] - (a + b) / 2; break;
				default: {
					int p = a + b - c, pa = abs(p - a), pb = abs(p - b), pc = abs(p - c);

					v = row[x] - (pa <= pb && pa <= pc ? a : pb <= pc ? b : c);
				}
				}
				v &= 255;
				sum += v < 128 ? v : 256 - v;
			}
			if (best < 0 || sum < best) {
				best = sum;
				bf = ft;
			}
		}
		*tmp++ = (uint8_t)bf;
		for (size_t x = 0; x < stride; x++) {
			int a = x >= 3 ? row[x - 3] : 0, b = up ? up[x] : 0, c = x >= 3 && up ? up[x - 3] : 0, v;

			switch (bf) {
			case 0: v = row[x]; break;
			case 1: v = row[x] - a; break;
			case 2: v = row[x] - b; break;
			case 3: v = row[x] - (a + b) / 2; break;
			default: {
				int p = a + b - c, pa = abs(p - a), pb = abs(p - b), pc = abs(p - c);

				v = row[x] - (pa <= pb && pa <= pc ? a : pb <= pc ? b : c);
			}
			}
			*tmp++ = (uint8_t)(v & 255);
		}
	}
}

static int write_png(const char *path, const uint8_t *rgb, int w, int h)
{
	size_t raw = ((size_t)w * 3 + 1) * (size_t)h;
	uint8_t *filt = malloc(raw), ihdr[13];
	struct out o = { 0 };
	uint32_t s1 = 1, s2 = 0;
	char tmp[1100];
	FILE *f;
	int bad;

	if (!filt)
		return -1;
	filter_rows(rgb, w, h, filt);
	put_byte(&o, 0x78);
	put_byte(&o, 0x01);
	deflate_fixed(&o, filt, raw);
	for (size_t i = 0; i < raw; i++) {
		s1 = (s1 + filt[i]) % 65521;
		s2 = (s2 + s1) % 65521;
	}
	free(filt);
	put_byte(&o, (uint8_t)(s2 >> 8));
	put_byte(&o, (uint8_t)s2);
	put_byte(&o, (uint8_t)(s1 >> 8));
	put_byte(&o, (uint8_t)s1);
	if (o.err) {
		free(o.p);
		return -1;
	}
	be32(ihdr, (uint32_t)w);
	be32(ihdr + 4, (uint32_t)h);
	ihdr[8] = 8;    /* bits por canal */
	ihdr[9] = 2;    /* RGB */
	ihdr[10] = ihdr[11] = ihdr[12] = 0;
	snprintf(tmp, sizeof tmp, "%s.%ld.tmp", path, (long)getpid());   /* dos generaciones a la vez no se pisan */
	f = fopen(tmp, "wb");
	if (!f) {
		free(o.p);
		return -1;
	}
	bad = fwrite("\x89PNG\r\n\x1a\n", 1, 8, f) != 8 || chunk(f, "IHDR", ihdr, 13) || chunk(f, "IDAT", o.p, o.n) ||
	      chunk(f, "IEND", NULL, 0);
	free(o.p);
	if (fclose(f) != 0 || bad || rename(tmp, path) != 0) {
		remove(tmp);
		return -1;
	}
	return 0;
}

/* ---- dibujo ---- */

struct col { double r, g, b; };

static struct col hexc(const char *hex)
{
	int r, g, b;

	rros_hex_rgb(hex, &r, &g, &b);
	return (struct col){ r / 255.0, g / 255.0, b / 255.0 };
}

static struct col mix(struct col a, struct col b, double t)
{
	t = t < 0 ? 0 : t > 1 ? 1 : t;
	return (struct col){ a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t };
}

/* fusión «trama» (screen): ilumina sin pasarse de blanco */
static struct col screen(struct col a, struct col b, double t)
{
	struct col s = { 1 - (1 - a.r) * (1 - b.r), 1 - (1 - a.g) * (1 - b.g), 1 - (1 - a.b) * (1 - b.b) };

	return mix(a, s, t);
}

static double lum(struct col c)
{
	return 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b;
}

static double smooth(double e0, double e1, double x)
{
	double t = (x - e0) / (e1 - e0);

	t = t < 0 ? 0 : t > 1 ? 1 : t;
	return t * t * (3 - 2 * t);
}

/* pseudoaleatorio determinista (el mismo fondo cada vez) */
static double rnd(uint32_t *s)
{
	*s = *s * 1664525u + 1013904223u;
	return (*s >> 8) / 16777216.0;
}

/* ruido de valor suave (determinista) para campos orgánicos */
static double hash2(int x, int y, uint32_t s)
{
	uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + s * 2246822519u;

	h = (h ^ (h >> 13)) * 1274126177u;
	return ((h ^ (h >> 16)) & 0xffffff) / 16777216.0;
}

static double vnoise(double x, double y, uint32_t s)
{
	double fx = floor(x), fy = floor(y), tx = x - fx, ty = y - fy;
	int ix = (int)fx, iy = (int)fy;
	double a = hash2(ix, iy, s), b = hash2(ix + 1, iy, s), c = hash2(ix, iy + 1, s), d = hash2(ix + 1, iy + 1, s);

	tx = tx * tx * tx * (tx * (tx * 6 - 15) + 10);
	ty = ty * ty * ty * (ty * (ty * 6 - 15) + 10);
	return a + (b - a) * tx + (c - a) * ty + (a - b - c + d) * tx * ty;
}

static double fbm(double x, double y, int oct)
{
	double s = 0, amp = 0.5, n = 0;

	for (int i = 0; i < oct; i++) {
		s += vnoise(x, y, (uint32_t)i + 7) * amp;
		n += amp;
		x = x * 2.03 + 17.1;
		y = y * 2.03 - 9.7;
		amp *= 0.5;
	}
	return s / n;
}

/* colores de la paleta y valores que solo dependen de la columna (se calculan una vez) */
struct ctx {
	struct col bg, sur, acc, mag, cya, blu, dim;
	int dark;
	int w, h;
	double aspect;
	double *col;    /* por columna: bandas de aurora, olas o dunas */
	double *fld;    /* campo por píxel (topográfico) */
	double *vtx;    /* vértices (x, y, altura) de la malla de cristal */
	int nx, ny;
	double shp[64][8];   /* formas sueltas: pétalos y discos desenfocados */
	int nshp;
};

#define NCOL 12

/* luz sobre el fondo: en temas oscuros «trama», en claros una mezcla más suave */
static struct col glow(const struct ctx *k, struct col c, struct col l, double t)
{
	return k->dark ? screen(c, l, t) : mix(c, l, t * 0.75);
}

static void ctx_init(struct ctx *k, const char *style, const struct rros_palette *p, int w, int h)
{
	uint32_t seed = 2024;

	memset(k, 0, sizeof *k);
	k->bg = hexc(p->bg);
	k->sur = hexc(p->surface);
	k->acc = hexc(p->accent);
	k->mag = hexc(p->magenta);
	k->cya = hexc(p->cyan);
	k->blu = hexc(p->blue);
	k->dim = hexc(p->accent_dim);
	k->dark = lum(k->bg) < 0.5;
	k->w = w;
	k->h = h;
	k->aspect = (double)w / h;
	k->col = calloc((size_t)w * NCOL, sizeof(double));
	if (!k->col)
		return;
	for (int x = 0; x < w; x++) {
		double u = (x + 0.5) / w, *c = k->col + (size_t)x * NCOL;

		if (!strcmp(style, "aurora")) {
			static const double prm[3][4] = { { 0.46, 0.07, 0.9, 0.1 }, { 0.55, 0.05, 1.4, 0.55 }, { 0.38, 0.06, 0.7, 0.8 } };

			for (int i = 0; i < 3; i++) {
				c[i * 2] = prm[i][0] + prm[i][1] * sin(2 * M_PI * (prm[i][2] * u + prm[i][3])) +
					   0.025 * sin(2 * M_PI * (3.1 * u + i));
				c[i * 2 + 1] = 0.7 + 0.3 * sin(2 * M_PI * (u * 46 + i * 0.3)) * sin(2 * M_PI * (u * 7.3 + i));
			}
		} else if (!strcmp(style, "ondas")) {
			for (int i = 0; i < 4; i++)
				c[i] = 0.52 + i * 0.11 + 0.045 * sin(2 * M_PI * (u * (1.1 + i * 0.35) + i * 0.27)) +
				       0.02 * sin(2 * M_PI * (u * 3.2 + i * 0.6));
		} else if (!strcmp(style, "dunas")) {
			/* cresta de cada duna (altura y pendiente); crestas algo afiladas */
			for (int i = 0; i < 5; i++) {
				double f = 0.8 + i * 0.35, ph = i * 0.37, e = 1e-3, hv[2];

				for (int s = 0; s < 2; s++) {
					double uu = u + s * e, q = 0.5 + 0.5 * sin(2 * M_PI * (f * uu + ph));

					hv[s] = 0.5 + i * 0.1 - (0.05 + 0.012 * i) * pow(q, 1.6) -
						0.012 * sin(2 * M_PI * (2.7 * uu + i * 0.8));
				}
				c[i * 2] = hv[0];
				c[i * 2 + 1] = (hv[1] - hv[0]) / e;
			}
		}
	}
	if (!strcmp(style, "petalos")) {
		/* seis pétalos de lirio desenfocados y discos de luz («bokeh») */
		static const double ang[6] = { -2.6, -1.95, -1.3, -0.62, 0.05, 0.75 };

		for (int i = 0; i < 6; i++) {
			double *s = k->shp[k->nshp++];

			s[0] = 0.74 * k->aspect;
			s[1] = 0.66;
			s[2] = ang[i] + 0.12 * sin(i * 2.1);
			s[3] = 0.38 + 0.08 * sin(i * 1.7 + 0.4);   /* largo */
			s[4] = 0.075 + 0.015 * cos(i * 2.3);       /* ancho */
			s[5] = 0.25 * sin(i * 1.3 + 0.5);          /* curvatura */
			s[6] = i & 1;
		}
		for (int i = 0; i < 40; i++) {
			double *s = k->shp[k->nshp++], r = rnd(&seed);

			s[0] = rnd(&seed) * k->aspect;
			s[1] = rnd(&seed);
			s[2] = 0.008 + 0.06 * r * r * r + 0.01 * rnd(&seed);   /* radio: pocos grandes */
			s[3] = 0.12 + 0.3 * rnd(&seed);                          /* desenfoque relativo */
			s[4] = (int)(rnd(&seed) * 3);
			s[5] = 0.1 + 0.22 * rnd(&seed) * (1 - r * 0.6);          /* intensidad */
		}
	} else if (!strcmp(style, "cristal")) {
		k->ny = 9;
		k->nx = (int)(k->ny * k->aspect + 0.5);
		k->nx = k->nx < 2 ? 2 : k->nx > 80 ? 80 : k->nx;
		k->vtx = malloc(sizeof(double) * 3 * (size_t)(k->nx + 1) * (size_t)(k->ny + 1));
		if (!k->vtx) {
			free(k->col);
			k->col = NULL;
			return;
		}
		for (int j = 0; j <= k->ny; j++)
			for (int i = 0; i <= k->nx; i++) {
				double *v = k->vtx + 3 * ((size_t)j * (k->nx + 1) + i), jx = rnd(&seed) - 0.5, jy = rnd(&seed) - 0.5;

				v[0] = (i + (i > 0 && i < k->nx ? jx * 0.7 : 0)) / k->nx * k->aspect;
				v[1] = (j + (j > 0 && j < k->ny ? jy * 0.7 : 0)) / k->ny;
				v[2] = rnd(&seed) * 0.09;
			}
	} else if (!strcmp(style, "topografico")) {
		k->fld = malloc(sizeof(double) * (size_t)w * h);
		if (!k->fld) {
			free(k->col);
			k->col = NULL;
			return;
		}
		for (int y = 0; y < h; y++)
			for (int x = 0; x < w; x++) {
				double u = (x + 0.5) / w * k->aspect, v = (y + 0.5) / h;
				double wx = fbm(u * 1.3 + 3.1, v * 1.3, 2) * 0.6, wy = fbm(u * 1.3, v * 1.3 + 5.2, 2) * 0.6;

				k->fld[(size_t)y * w + x] = fbm(u * 1.6 + wx, v * 1.6 + wy, 4);
			}
	}
}

/* pétalo: elipse afilada en la punta, curvada; distancia con signo aproximada a su borde */
static double petal_dist(const double *s, double x, double y, double *along)
{
	double dx = x - s[0], dy = y - s[1], ca = cos(s[2]), sa = sin(s[2]);
	double a = dx * ca + dy * sa, b = -dx * sa + dy * ca, t = a / s[3], hw;

	*along = t;
	b -= s[5] * s[3] * t * t;
	if (t <= 0)
		return sqrt(a * a + b * b);
	if (t >= 1)
		return sqrt((a - s[3]) * (a - s[3]) + b * b);
	hw = s[4] * pow(sin(M_PI * pow(t, 0.75)), 0.9);
	return fabs(b) - hw;
}

static struct col px_petalos(const struct ctx *k, double u, double v)
{
	double x = u * k->aspect, gx = (u - 0.3) * k->aspect, gy = v - 0.3;
	struct col pal[3] = { k->acc, k->mag, k->cya };
	struct col c = mix(mix(k->sur, k->bg, 0.25), k->bg, smooth(0.0, 1.3, sqrt(gx * gx + gy * gy)));

	/* halo detrás de la flor */
	{
		double hx = x - k->shp[0][0], hy = v - k->shp[0][1];

		c = glow(k, c, k->acc, exp(-(hx * hx + hy * hy) / 0.09) * (k->dark ? 0.32 : 0.22));
	}
	for (int i = 6; i < k->nshp; i++) {
		const double *s = k->shp[i];
		double dx = x - s[0], dy = v - s[1], d = sqrt(dx * dx + dy * dy), r = s[2], bl = r * s[3] + 0.0015;

		if (d < r + bl) {
			double in = 1 - smooth(r - bl, r + bl, d), rim = 0.75 + 0.25 * smooth(r * 0.4, r, d);

			c = glow(k, c, pal[(int)s[4]], in * rim * s[5]);
		}
	}
	for (int i = 0; i < 6; i++) {
		const double *s = k->shp[i];
		double t, d = petal_dist(s, x, v, &t), bl = 0.006 + 0.03 * (s[6] ? 1 : 0.4);

		if (d < bl) {
			double a = 1 - smooth(-bl, bl, d), tt = t < 0 ? 0 : t > 1 ? 1 : t;
			struct col pc = mix(mix(k->acc, k->mag, tt * 0.8), k->cya, smooth(0.75, 1.0, tt) * 0.5);

			/* nervio central más claro */
			pc = mix(pc, k->cya, exp(-pow(d / (s[4] * 0.35) + 1, 2) * 4) * 0.0 + (d < 0 ? 0.12 * (1 + d / s[4]) : 0));
			c = mix(c, pc, a * (s[6] ? 0.55 : 0.8) * (0.55 + 0.45 * (1 - tt)) * (k->dark ? 1 : 0.85));
		}
	}
	return c;
}

static struct col px_dunas(const struct ctx *k, const double *cv, double u, double v)
{
	double pv = 1.0 / k->h, sx = (u - 0.68) * k->aspect, sy = v - 0.36, sd = sqrt(sx * sx + sy * sy);
	struct col haze = mix(k->sur, k->acc, k->dark ? 0.45 : 0.3);
	struct col c = mix(k->dark ? mix(k->bg, (struct col){ 0, 0, 0 }, 0.25) : mix(k->bg, k->blu, 0.12), haze,
			   smooth(0.0, 0.62, v));

	/* sol bajo con halo */
	c = glow(k, c, k->cya, exp(-sd * sd / 0.03) * 0.35);
	c = glow(k, c, mix(k->cya, (struct col){ 1, 1, 1 }, 0.3), 1 - smooth(0.05, 0.053, sd));
	for (int i = 0; i < 5; i++) {
		double top = cv[i * 2], slope = cv[i * 2 + 1], e = smooth(top - pv, top + pv, v);

		if (e > 0) {
			double depth = i / 4.0, light = 0.5 + 0.5 * tanh(-slope * 4);
			struct col far = mix(haze, k->acc, 0.25), near = k->dark ? mix(k->bg, k->sur, 0.3) : mix(k->sur, k->dim, 0.35);
			struct col dc = mix(far, near, pow(depth, 0.8));

			dc = mix(dc, mix(dc, k->bg, 0.6), (1 - light) * 0.5);
			dc = glow(k, dc, k->cya, light * 0.12 * (1 - depth * 0.5));
			/* brillo fino en la cresta y sombra hacia el pie */
			dc = glow(k, dc, k->cya, exp(-pow((v - top) / (pv * 3 + 0.002), 2)) * 0.25 * light);
			dc = mix(dc, k->bg, smooth(top, top + 0.25, v) * 0.35);
			c = mix(c, dc, e);
		}
	}
	return c;
}

static struct col px_malla(const struct ctx *k, double u, double v)
{
	static const double pt[6][3] = { { 0.12, 0.18, 0.11 }, { 0.82, 0.12, 0.09 }, { 0.62, 0.82, 0.12 },
					 { 0.18, 0.88, 0.08 }, { 0.48, 0.45, 0.06 }, { 0.95, 0.62, 0.07 } };
	struct col pal[6] = { k->acc, k->mag, k->cya, k->blu, k->dim, k->acc };
	double wu = u + 0.07 * sin(2 * M_PI * (v * 1.2 + 0.15)) + 0.03 * sin(2 * M_PI * (v * 2.7 + u)),
	       wv = v + 0.07 * sin(2 * M_PI * (u * 1.1 + 0.55)) + 0.03 * sin(2 * M_PI * (u * 2.3 - v));
	double sw = 0.35, r = k->bg.r * sw, g = k->bg.g * sw, b = k->bg.b * sw;

	for (int i = 0; i < 6; i++) {
		double dx = (wu - pt[i][0]) * k->aspect * 0.8, dy = wv - pt[i][1], wt = exp(-(dx * dx + dy * dy) / pt[i][2]);

		r += pal[i].r * wt;
		g += pal[i].g * wt;
		b += pal[i].b * wt;
		sw += wt;
	}
	return mix((struct col){ r / sw, g / sw, b / sw }, k->bg, k->dark ? 0.32 : 0.25);
}

/* cristal: base en degradado y triángulos con luz según su inclinación */
static struct col cristal_base(const struct ctx *k, double u, double v)
{
	double dx = (u - 0.8) * k->aspect, dy = v - 0.2, ex = (u - 0.15) * k->aspect, ey = v - 0.9;
	struct col c = mix(mix(k->sur, k->bg, 0.2), k->bg, u * 0.4 + v * 0.6);

	c = glow(k, c, k->acc, exp(-(dx * dx + dy * dy) / 0.35) * (k->dark ? 0.6 : 0.45));
	return glow(k, c, k->mag, exp(-(ex * ex + ey * ey) / 0.25) * (k->dark ? 0.35 : 0.3));
}

static int tri_hit(const double *a, const double *b, const double *c, double x, double y)
{
	double d1 = (x - b[0]) * (a[1] - b[1]) - (a[0] - b[0]) * (y - b[1]);
	double d2 = (x - c[0]) * (b[1] - c[1]) - (b[0] - c[0]) * (y - c[1]);
	double d3 = (x - a[0]) * (c[1] - a[1]) - (c[0] - a[0]) * (y - a[1]);

	return !((d1 < 0 || d2 < 0 || d3 < 0) && (d1 > 0 || d2 > 0 || d3 > 0));
}

static struct col cristal_at(const struct ctx *k, double x, double y)
{
	int ci = (int)(x / k->aspect * k->nx), cj = (int)(y * k->ny);

	for (int j = cj - 1; j <= cj + 1; j++)
		for (int i = ci - 1; i <= ci + 1; i++) {
			const double *q[4];

			if (i < 0 || j < 0 || i >= k->nx || j >= k->ny)
				continue;
			q[0] = k->vtx + 3 * ((size_t)j * (k->nx + 1) + i);
			q[1] = q[0] + 3;
			q[2] = q[0] + 3 * (k->nx + 1);
			q[3] = q[2] + 3;
			for (int t = 0; t < 2; t++) {
				const double *a, *b, *c;

				/* la diagonal alterna para que no se vea una rejilla */
				if ((i + j) & 1) {
					a = t ? q[1] : q[0]; b = t ? q[3] : q[1]; c = q[2];
				} else {
					a = t ? q[0] : q[0]; b = t ? q[3] : q[1]; c = t ? q[2] : q[3];
				}
				if (tri_hit(a, b, c, x, y)) {
					double ux = b[0] - a[0], uy = b[1] - a[1], uz = b[2] - a[2];
					double vx = c[0] - a[0], vy = c[1] - a[1], vz = c[2] - a[2];
					double nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx, nl;
					double cx = (a[0] + b[0] + c[0]) / 3, cy = (a[1] + b[1] + c[1]) / 3, sh;
					struct col base = cristal_base(k, cx / k->aspect, cy);

					if (nz < 0) {
						nx = -nx; ny = -ny; nz = -nz;
					}
					nl = sqrt(nx * nx + ny * ny + nz * nz);
					/* luz desde arriba a la derecha */
					sh = (nx * 0.55 - ny * 0.55 + nz * 0.63) / nl - 0.63;
					base = sh > 0 ? glow(k, base, k->cya, sh * 2.2) : mix(base, k->dark ? (struct col){ 0, 0, 0 } : k->bg, -sh * 2.5);
					return base;
				}
			}
		}
	return cristal_base(k, x / k->aspect, y);
}

static struct col px_cristal(const struct ctx *k, double u, double v)
{
	double ox = 0.25 / k->w * k->aspect, oy = 0.25 / k->h, x = u * k->aspect;
	struct col a = cristal_at(k, x - ox, v - oy), b = cristal_at(k, x + ox, v - oy), c = cristal_at(k, x - ox, v + oy),
		   d = cristal_at(k, x + ox, v + oy);

	return (struct col){ (a.r + b.r + c.r + d.r) / 4, (a.g + b.g + c.g + d.g) / 4, (a.b + b.b + c.b + d.b) / 4 };
}

static struct col px_topografico(const struct ctx *k, int x, int y, double u, double v)
{
	const double *f = k->fld + (size_t)y * k->w + x;
	double fx = (x + 1 < k->w ? f[1] : f[0]) - (x > 0 ? f[-1] : f[0]);
	double fy = (y + 1 < k->h ? f[k->w] : f[0]) - (y > 0 ? f[-k->w] : f[0]);
	double gr = sqrt(fx * fx + fy * fy) / 2 + 1e-9, n = 38, l = f[0] * n, fr = l - floor(l), d = fmin(fr, 1 - fr) / (gr * n);
	int major = ((int)floor(l + 0.5)) % 5 == 0;
	double wd = major ? 1.1 : 0.55, line = 1 - smooth(wd - 0.6, wd + 0.6, d);
	double dx = (u - 0.75) * k->aspect, dy = v - 0.3;
	struct col c = mix(mix(k->sur, k->bg, 0.3), k->bg, smooth(0.0, 1.0, u * 0.3 + v * 0.7));
	struct col lc = mix(k->acc, k->cya, smooth(0.35, 0.7, f[0]));

	c = glow(k, c, k->acc, exp(-(dx * dx + dy * dy) / 0.25) * (k->dark ? 0.25 : 0.18));
	if (line > 0)
		c = mix(c, lc, line * (major ? 0.7 : 0.35) * (k->dark ? 1 : 0.9));
	return c;
}

static struct col px_seda(const struct ctx *k, double u, double v)
{
	static const double rb[3][7] = { { 0.62, 0.16, 0.55, 0.05, 0.16, 0.06, 0.8 },
					 { 0.42, 0.12, 0.8, 0.45, 0.11, 0.04, 1.3 },
					 { 0.78, 0.08, 1.1, 0.7, 0.07, 0.025, 1.9 } };
	struct col c = mix(mix(k->sur, k->bg, 0.15), k->bg, smooth(0.0, 1.0, v * 0.6 + (1 - u) * 0.4));
	double pv = 1.5 / k->h;

	for (int i = 0; i < 3; i++) {
		const double *r = rb[i];
		double ctr = r[0] + r[1] * sin(2 * M_PI * (r[2] * u + r[3])) + 0.03 * sin(2 * M_PI * (2.1 * u + i));
		double th = r[4] + r[5] * sin(2 * M_PI * (r[6] * u + 0.3 * i)), s = (v - ctr) / th, as = fabs(s);

		if (as < 1 + pv / th) {
			/* pliegues: la tela gira a lo largo de la cinta */
			double tw = 2.2 * s + 3.0 * sin(2 * M_PI * (0.6 * u + 0.2 * i)) + 1.5 * u;
			double sh = 0.5 + 0.5 * cos(tw), spec = pow(sh, 14), a = 1 - smooth(1 - pv / th, 1 + pv / th, as);
			struct col rc = mix(mix(k->acc, k->mag, (s + 1) / 2), k->dim, (1 - sh) * 0.55);

			rc = mix(rc, k->bg, (1 - sh) * 0.45);
			rc = glow(k, rc, k->cya, spec * 0.55 + 0.04 * sin(s * 90) * sh);
			c = mix(c, rc, a * (0.92 - 0.2 * i) * (0.7 + 0.3 * (1 - as * as)));
		}
	}
	return c;
}

static struct col pixel(const char *style, const struct ctx *k, int x, int y, double u, double v)
{
	const double *cv = k->col + (size_t)x * NCOL;
	struct col c;

	if (!strcmp(style, "petalos"))
		return px_petalos(k, u, v);
	if (!strcmp(style, "dunas"))
		return px_dunas(k, cv, u, v);
	if (!strcmp(style, "malla"))
		return px_malla(k, u, v);
	if (!strcmp(style, "cristal"))
		return px_cristal(k, u, v);
	if (!strcmp(style, "topografico"))
		return px_topografico(k, x, y, u, v);
	if (!strcmp(style, "seda"))
		return px_seda(k, u, v);
	if (!strcmp(style, "ondas")) {
		struct col layers[4] = { k->dim, k->blu, k->acc, k->mag };
		static const double alpha[4] = { 0.55, 0.45, 0.6, 0.5 };

		c = mix(k->bg, k->sur, v * 0.9);
		for (int i = 0; i < 4; i++) {
			double edge = smooth(cv[i] - 0.002, cv[i] + 0.002, v);

			if (edge > 0)
				c = mix(c, mix(layers[i], k->bg, k->dark ? 0.25 : 0.1), edge * alpha[i]);
		}
		return c;
	}
	if (!strcmp(style, "degradado")) {
		double dx = (u - 0.82) * k->aspect, dy = v - 0.18, ex = (u - 0.12) * k->aspect, ey = v - 0.88;

		c = mix(mix(k->sur, k->bg, 0.35), k->bg, u * 0.55 + v * 0.45);
		c = screen(c, k->acc, exp(-(dx * dx + dy * dy) / 0.16) * (k->dark ? 0.55 : 0.4));
		c = screen(c, k->mag, exp(-(ex * ex + ey * ey) / 0.12) * (k->dark ? 0.4 : 0.3));
		return c;
	}
	if (!strcmp(style, "circulos")) {
		static const double cs[7][4] = { { 0.15, 0.25, 0.32, 0 }, { 0.78, 0.2, 0.26, 1 }, { 0.55, 0.78, 0.38, 2 },
						 { 0.92, 0.85, 0.2, 0 }, { 0.32, 0.62, 0.14, 1 }, { 0.65, 0.45, 0.1, 2 },
						 { 0.05, 0.9, 0.22, 1 } };
		struct col pal[3] = { k->acc, k->mag, k->cya };

		c = mix(k->bg, k->sur, v * 0.7);
		for (int i = 0; i < 7; i++) {
			double dx = (u - cs[i][0]) * k->aspect, dy = v - cs[i][1], d = sqrt(dx * dx + dy * dy);
			double ring = (d - cs[i][2]) / 0.012;

			if (d < cs[i][2] + 0.004)
				c = mix(c, pal[(int)cs[i][3]], (1 - smooth(cs[i][2] - 0.004, cs[i][2] + 0.004, d)) * (k->dark ? 0.16 : 0.12));
			if (ring > -4 && ring < 4)
				c = screen(c, pal[(int)cs[i][3]], exp(-ring * ring) * 0.25);
		}
		return c;
	}
	/* aurora: cortinas de luz sobre un cielo en degradado */
	{
		struct col sky_top = k->dark ? mix(k->bg, (struct col){ 0, 0, 0 }, 0.35) : mix(k->bg, k->sur, 0.2);
		struct col bands[3] = { k->acc, k->dark ? k->cya : k->blu, k->mag };
		static const double width[3] = { 0.07, 0.05, 0.045 };

		c = mix(sky_top, k->sur, smooth(0.0, 1.1, v));
		for (int i = 0; i < 3; i++) {
			double d = (v - cv[i * 2]) / width[i];
			/* borde inferior nítido; hacia arriba se difumina, como una cortina */
			double q = d > 0 ? d * d * 2.2 : d * d * 0.18;

			if (q < 12)
				c = screen(c, bands[i], exp(-q) * cv[i * 2 + 1] * (k->dark ? 0.62 : 0.45));
		}
		/* resplandor en el horizonte */
		if (v > 0.5)
			c = screen(c, mix(k->dim, k->acc, 0.5), exp(-pow((v - 1.02) / 0.22, 2)) * 0.3);
		return c;
	}
}

int rros_wallpaper_render(const char *style, const struct rros_palette *p, int w, int h, const char *path)
{
	static const int bayer[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };
	uint8_t *img;
	uint32_t seed = 12345;
	struct ctx k;
	int rc;

	if (w < 16 || h < 16 || w > 8192 || h > 8192)
		return -1;
	img = malloc((size_t)w * h * 3);
	ctx_init(&k, style, p, w, h);
	if (!img || !k.col) {
		free(img);
		free(k.col);
		return -1;
	}
	for (int y = 0; y < h; y++)
		for (int x = 0; x < w; x++) {
			struct col c = pixel(style, &k, x, y, (x + 0.5) / w, (y + 0.5) / h);
			double dith = (bayer[y & 3][x & 3] + 0.5) / 16.0 - 0.5;
			double ch[3] = { c.r, c.g, c.b };

			for (int k = 0; k < 3; k++) {
				double v = ch[k] * 255 + dith;

				img[((size_t)y * w + x) * 3 + k] = (uint8_t)(v < 0 ? 0 : v > 255 ? 255 : v + 0.5);
			}
		}
	/* estrellas en el cielo de la aurora (solo en temas oscuros) */
	free(k.col);
	free(k.fld);
	free(k.vtx);
	if (!strcmp(style, "aurora") && k.dark)
		for (int i = 0; i < w * h / 9000; i++) {
			int x = (int)(rnd(&seed) * w), y = (int)(rnd(&seed) * h * 0.55);
			double b = 0.35 + rnd(&seed) * 0.5;
			uint8_t *px = img + ((size_t)y * w + x) * 3;

			for (int k = 0; k < 3; k++)
				px[k] = (uint8_t)(px[k] + (255 - px[k]) * b);
		}
	rc = write_png(path, img, w, h);
	free(img);
	return rc;
}

void rros_wallpaper_path(const struct rros_theme *t, const struct rros_palette *p, char *out, size_t n)
{
	const char *x = getenv("XDG_DATA_HOME"), *home = getenv("HOME");
	uint32_t hsh = 2166136261u;
	const char *parts[7] = { p->bg, p->surface, p->accent, p->accent_dim, p->magenta, p->cyan, p->blue };

	*out = '\0';
	if (t->wallpaper[0] && access(t->wallpaper, R_OK) == 0) {
		snprintf(out, n, "%s", t->wallpaper);
		return;
	}
	if (!strcmp(t->wallpaper_style, "liso"))
		return;
	/* el nombre cambia con los colores: cambiar de tema o de acento genera un fondo nuevo */
	for (int i = 0; i < 7; i++)
		for (const char *c = parts[i]; *c; c++)
			hsh = (hsh ^ (unsigned char)*c) * 16777619u;
	for (const char *c = t->wallpaper_style; *c; c++)
		hsh = (hsh ^ (unsigned char)*c) * 16777619u;
	if (x && *x)
		snprintf(out, n, "%s/rros/fondos/%s-%08x.png", x, t->wallpaper_style, hsh);
	else if (home && *home)
		snprintf(out, n, "%s/.local/share/rros/fondos/%s-%08x.png", home, t->wallpaper_style, hsh);
}
