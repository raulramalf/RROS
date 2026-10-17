/* Fondos de pantalla generados con la paleta del tema (PNG, sin librerías externas).
 * Estilos: aurora, ondas, degradado, circulos (liso = sin imagen, solo el color de fondo). */
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
	snprintf(tmp, sizeof tmp, "%s.tmp", path);
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

/* colores de la paleta y valores que solo dependen de la columna (se calculan una vez) */
struct ctx {
	struct col bg, sur, acc, mag, cya, blu, dim;
	int dark;
	double aspect;
	double *col;    /* por columna: 3 bandas de aurora (centro, brillo) o 4 olas */
};

static void ctx_init(struct ctx *k, const char *style, const struct rros_palette *p, int w, double aspect)
{
	k->bg = hexc(p->bg);
	k->sur = hexc(p->surface);
	k->acc = hexc(p->accent);
	k->mag = hexc(p->magenta);
	k->cya = hexc(p->cyan);
	k->blu = hexc(p->blue);
	k->dim = hexc(p->accent_dim);
	k->dark = lum(k->bg) < 0.5;
	k->aspect = aspect;
	k->col = calloc((size_t)w * 6, sizeof(double));
	if (!k->col)
		return;
	for (int x = 0; x < w; x++) {
		double u = (x + 0.5) / w, *c = k->col + (size_t)x * 6;

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
		}
	}
}

static struct col pixel(const char *style, const struct ctx *k, int x, double u, double v)
{
	const double *cv = k->col + (size_t)x * 6;
	struct col c;

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
	ctx_init(&k, style, p, w, (double)w / h);
	if (!img || !k.col) {
		free(img);
		free(k.col);
		return -1;
	}
	for (int y = 0; y < h; y++)
		for (int x = 0; x < w; x++) {
			struct col c = pixel(style, &k, x, (x + 0.5) / w, (y + 0.5) / h);
			double dith = (bayer[y & 3][x & 3] + 0.5) / 16.0 - 0.5;
			double ch[3] = { c.r, c.g, c.b };

			for (int k = 0; k < 3; k++) {
				double v = ch[k] * 255 + dith;

				img[((size_t)y * w + x) * 3 + k] = (uint8_t)(v < 0 ? 0 : v > 255 ? 255 : v + 0.5);
			}
		}
	/* estrellas en el cielo de la aurora (solo en temas oscuros) */
	free(k.col);
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
