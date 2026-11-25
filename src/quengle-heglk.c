/* quengle: heglk.c with pictures (also in hugo windows, as overlays) and sound. emglken compiles heglk.c's media stubs; gargoyle's hemedia.c writes each
   resource to a PIC<n>/SND<n> file for glk to find, which remglk-rs can't. here the bytes go straight to remglk-rs
   (quengle_add_image/quengle_add_sound, see remglk.patch), which hands them to the display. compiled in place of
   heglk.c, which it includes, since that keeps mainwin/currentwin static */

#define GRAPHICS_SUPPORTED
#define SOUND_SUPPORTED
/* wrapped below: clearing also clears overlay pictures */
#define hugo_clearfullscreen heglk_clearfullscreen
#define hugo_clearwindow heglk_clearwindow
#define hugo_settextwindow heglk_settextwindow
#define heglk_get_linelength heglk_get_linelength_orig
#define heglk_get_screenheight heglk_get_screenheight_orig
/* text in floating windows (see quengle_print below) */
#define hugo_print heglk_print
#define hugo_settextpos heglk_settextpos
/* heglk's video stubs (engine 3.x, i.e. without COMPILE_V25): replaced below */
#define hugo_hasvideo heglk_hasvideo
#define hugo_playvideo heglk_playvideo
#define hugo_stopvideo heglk_stopvideo
#include "heglk.c"
#undef hugo_hasvideo
#undef hugo_playvideo
#undef hugo_stopvideo
#undef heglk_get_linelength
#undef heglk_get_screenheight
#undef hugo_print
#undef hugo_settextpos
#undef hugo_clearfullscreen
#undef hugo_clearwindow
#undef hugo_settextwindow

glui32 quengle_add_image(const unsigned char *buf, glui32 len);
glui32 quengle_add_sound(const unsigned char *buf, glui32 len);
void quengle_overlay(glui32 op, glui32 id, glui32 left, glui32 top, glui32 right, glui32 bottom);
void quengle_video_data(glui32 id, const unsigned char *buf, glui32 len);
void quengle_overlay_text(glui32 left, glui32 top, glui32 right, glui32 bottom, glui32 cols, glui32 rows,
	glui32 col, glui32 row, glui32 fg, glui32 bg, glui32 font, const char *text, glui32 len);
glui32 quengle_video_busy(void);

/* pictures in a hugo `window` (title screens: logos, mugshots side by side) have no glk equivalent: glk windows
   tile, they can't sit anywhere. the display lays these over the game instead, at the window's place as a fraction
   (0-10000) of the screen size the game last read (display.screenwidth/height), which its layout maths is based
   on. a clear removes them */
#define OVERLAY_CLEAR 0
#define OVERLAY_PICTURE 1
#define OVERLAY_CLEAR_RECT 2
/* the main text area. after window blocks the engine narrows it (below a room picture under the status line;
   beside a portrait column), which glk can't express: the display moves the main window there instead */
#define OVERLAY_MAIN 3
/* a video in the window (Future Boy!: cinepak avi). id packs the clip (low 16 bits), loop (bit 16) and volume
   0-100 (bits 17-23). VIDEO_STOP ends any */
#define OVERLAY_VIDEO 4
#define OVERLAY_VIDEO_STOP 5
/* a picture in a window over the main text (blank lines printed for it first: Future Boy!'s chapter headings):
   it belongs to the text and scrolls with it. a marker character goes into mainwin where it was drawn */
#define OVERLAY_TEXT_PICTURE 7
#define TEXT_PICTURE_MARK 0x2063
/* window clears carry a colour: id = hugo colour + 1 when the window's background differs from the main window's
   (Cryptozookeeper's purple panel), else 0 (transparent: the theme shows through) */

/* the screen size the game last read (see heglk_get_linelength below) */
static int seen_width = 80, seen_height = 25;
static glui32 fraction(int pos, int size);
static glui32 main_rect[4] = { 0, 0, 10000, 10000 };
/* first row of the last main text area with room in it (a window block parks the main area off screen for its
   duration): pictures in windows below it are drawn over the text */
static int text_top = 1;
/* line ends printed to mainwin since its last text: a game makes room for a picture in the text that way */
static int blank_lines = 0;

/* the current hugo window, in characters, 1-based inclusive. kept here because heglk drops floating windows
   before it updates physical_window* */
static int window_left = 1, window_top = 1, window_right = 0x7fff, window_bottom = 0x7fff;

/* our cursor in a floating window, 1-based. heglk drops text there (glk can't place such a window) */
static int float_col = 1, float_row = 1;

void hugo_settextwindow(int left, int top, int right, int bottom)
{
	float_col = float_row = 1;
	window_left = left;
	window_top = top;
	window_right = right;
	window_bottom = bottom;
	heglk_settextwindow(left, top, right, bottom);

	/* outside a window block, this is the main text area */
	if (!inwindow)
	{
		/* heglk calls a main area that ends above the previous window's bottom "floating" and drops what's
		   printed there (Cryptozookeeper's intro and room text). it's the main text: mainwin, which the display
		   moves to this rect */
		if (currentwin == NULL) glk_set_window(currentwin = mainwin);
		if (bottom - top > 3) text_top = top;
		glui32 r[4] = { fraction(left - 1, seen_width), fraction(top - 1, seen_height),
			fraction(right, seen_width), fraction(bottom, seen_height) };
		if (memcmp(r, main_rect, sizeof r))
		{
			memcpy(main_rect, r, sizeof r);
			quengle_overlay(OVERLAY_MAIN, 0, r[0], r[1], r[2], r[3]);
		}
	}
}

int heglk_get_linelength(void)
{
	int w = heglk_get_linelength_orig();
	if (w > 0) seen_width = w;
	return w;
}

int heglk_get_screenheight(void)
{
	/* heglk leaves out auxwin, the grid it opens under mainwin for positioned text, which takes mainwin's room:
	   the screen seemed to shrink, and windows laid out on the old height overflowed */
	int h = heglk_get_screenheight_orig();
	if (auxwin)
	{
		glui32 aux = 0;
		glk_window_get_size(auxwin, NULL, &aux);
		h += aux;
	}
	/* each glk window's rows round down on their own, so the sum wobbles by a row as windows open and close,
	   and panels placed on one reading didn't meet a main area placed on the next. the height measured with
	   mainwin alone holds while readings stay within rounding of it; a bigger change is a real resize */
	static int whole = 0;
	if (!secondwin && !auxwin) whole = h;
	else if (whole && abs(h - whole) <= 2) h = whole;
	if (h > 0) seen_height = h;
	return h;
}

static glui32 fraction(int pos, int size)
{
	if (pos < 0) pos = 0;
	if (pos > size) pos = size;
	return (glui32)((long long)pos * 10000 / size);
}

static void quengle_overlay_rect(glui32 op, glui32 id)
{
	quengle_overlay(op, id,
		fraction(window_left - 1, seen_width), fraction(window_top - 1, seen_height),
		fraction(window_right, seen_width), fraction(window_bottom, seen_height));
}

void hugo_clearfullscreen(void)
{
	blank_lines = 0;
	quengle_overlay(OVERLAY_CLEAR, 0, 0, 0, 0, 0);
	heglk_clearfullscreen();
}

void hugo_clearwindow(void)
{
	/* only floating windows get a colour: a real glk window (the status line) shows its own */
	if (inwindow) quengle_overlay_rect(OVERLAY_CLEAR_RECT,
		currentwin == NULL && glk_bgcolor != mainwin_bgcolor ? (glui32)glk_bgcolor + 1 : 0);
	else quengle_overlay(OVERLAY_CLEAR, 0, 0, 0, 0, 0);
	float_col = float_row = 1;
	heglk_clearwindow();
}

void hugo_settextpos(int x, int y)
{
	float_col = x;
	float_row = y;
	/* heglk takes positioned fixed-width text in the main window for a menu and opens auxwin for it, except at
	   the bottom of the screen; closing auxwin later clears mainwin. it measures "bottom" against the whole
	   screen, so a main area below window panels (Cryptozookeeper parks the cursor at its bottom so text scrolls
	   up from there) looked like a menu and lost its text. measured against the main area instead */
	if (!inwindow && currentwin == mainwin && y > window_bottom - window_top + 1 - 0x0f)
	{
		just_cleared_screen = false;
		currentline = y;
		currentpos = (x - 1) * CHARWIDTH;
		return;
	}
	heglk_settextpos(x, y);
}

/* text for a floating window goes to the display as a run at our cursor, with its colours and font; the display
   draws it in the frame buffer on the window's grid of cells */
void hugo_print(char *a)
{
	if (currentwin == NULL && inwindow)
	{
		glui32 cols = window_right - window_left + 1, rows = window_bottom - window_top + 1;
		/* line by line: a run may carry its own line ends */
		while (*a)
		{
			glui32 len = strcspn(a, "\n\r");
			if (len && float_row >= 1 && (glui32)float_row <= rows)
			{
				quengle_overlay_text(fraction(window_left - 1, seen_width), fraction(window_top - 1, seen_height),
					fraction(window_right, seen_width), fraction(window_bottom, seen_height),
					cols, rows, float_col, float_row, glk_fcolor,
					glk_bgcolor != mainwin_bgcolor ? (glui32)glk_bgcolor + 1 : 0, glk_current_font, a, len);
			}
			float_col += len;
			a += len;
			if (*a)
			{
				float_col = 1;
				float_row++;
				a++;
			}
		}
		return;
	}
	if (currentwin == mainwin && !inwindow)
	{
		if (a[0] == '\n' || a[0] == '\r') blank_lines++;
		else if (a[0]) blank_lines = 0;
	}
	heglk_print(a);
}

#ifndef MAX_RES_PATH
#define MAX_RES_PATH 255 /* as heres.c */
#endif
#define MAX_RESOURCES 512

/* the same resource is read again whenever it's used: register it once per (file, resource) */
static struct {
	char file[MAX_RES_PATH];
	char res[MAX_RES_PATH];
	int sound;
	glui32 id;
} resources[MAX_RESOURCES];
static int resource_count = 0;

/* the resource's glk number, 0 if unusable. closes infile */
static glui32 quengle_resource(HUGO_FILE infile, long reslength, int sound)
{
	glui32 id = 0;
	int i;

	for (i = 0; i < resource_count; i++)
	{
		if (resources[i].sound == sound && !strcmp(resources[i].file, loaded_filename)
			&& !strcmp(resources[i].res, loaded_resname))
		{
			fclose(infile);
			return resources[i].id;
		}
	}

	if (reslength > 0)
	{
		unsigned char *buf = malloc(reslength);
		if (buf)
		{
			glui32 got = glk_get_buffer_stream(infile, (char *)buf, reslength);
			if (got == (glui32)reslength)
				id = sound ? quengle_add_sound(buf, got) : quengle_add_image(buf, got);
			free(buf);
		}
	}
	fclose(infile);

	if (id && resource_count < MAX_RESOURCES)
	{
		strcpy(resources[resource_count].file, loaded_filename);
		strcpy(resources[resource_count].res, loaded_resname);
		resources[resource_count].sound = sound;
		resources[resource_count++].id = id;
	}
	return id;
}

int hugo_hasgraphics(void)
{
	return glk_gestalt(gestalt_Graphics, 0)
		&& glk_gestalt(gestalt_DrawImage, glk_window_get_type(mainwin));
}

int hugo_displaypicture(HUGO_FILE infile, long reslength)
{
	glui32 id = quengle_resource(infile, reslength, false);
	if (!id) return false;

	if (inwindow)
	{
		if (window_top > text_top && blank_lines >= 2)
		{
			glk_put_char_stream_uni(glk_window_get_stream(mainwin), TEXT_PICTURE_MARK);
			quengle_overlay_rect(OVERLAY_TEXT_PICTURE, id);
		}
		else quengle_overlay_rect(OVERLAY_PICTURE, id);
		return true;
	}
	if (glk_image_draw(mainwin, id, imagealign_InlineUp, 0))
		glk_put_char('\n');
	return true;
}

/* hugo has one music and one sample channel */
static schanid_t music_channel = NULL;
static schanid_t sample_channel = NULL;

static schanid_t quengle_channel(int music)
{
	schanid_t *chan = music ? &music_channel : &sample_channel;
	if (!*chan && glk_gestalt(gestalt_Sound, 0))
		*chan = glk_schannel_create(0);
	return *chan;
}

static int quengle_play(HUGO_FILE infile, long reslength, char loop_flag, int music)
{
	schanid_t chan = quengle_channel(music);
	glui32 id;

	if (!chan)
	{
		fclose(infile);
		return true; /* no sound here: not the game's error */
	}
	id = quengle_resource(infile, reslength, true);
	if (!id) return false;
	return glk_schannel_play_ext(chan, id, loop_flag ? (glui32)-1 : 1, 0) != 0;
}

int hugo_playmusic(HUGO_FILE infile, long reslength, char loop_flag)
{
	return quengle_play(infile, reslength, loop_flag, true);
}

int hugo_playsample(HUGO_FILE infile, long reslength, char loop_flag)
{
	return quengle_play(infile, reslength, loop_flag, false);
}

/* hugo volumes are 0-100 */
void hugo_musicvolume(int vol)
{
	schanid_t chan = quengle_channel(true);
	if (chan) glk_schannel_set_volume(chan, (glui32)vol * 0x10000 / 100);
}

void hugo_samplevolume(int vol)
{
	schanid_t chan = quengle_channel(false);
	if (chan) glk_schannel_set_volume(chan, (glui32)vol * 0x10000 / 100);
}

void hugo_stopmusic(void)
{
	if (music_channel) glk_schannel_stop(music_channel);
}

void hugo_stopsample(void)
{
	if (sample_channel) glk_schannel_stop(sample_channel);
}

/* video: the clip's bytes go to the display once (quengle_video_data), then play as an overlay at the current
   window. without `background` the game waits until the clip ends; a key skips it */
#define MAX_VIDEOS 128
static struct {
	char file[MAX_RES_PATH];
	char res[MAX_RES_PATH];
} videos[MAX_VIDEOS];
static int video_count = 0;

int hugo_hasvideo(void)
{
	return hugo_hasgraphics();
}

void hugo_stopvideo(void)
{
	quengle_overlay(OVERLAY_VIDEO_STOP, 0, 0, 0, 0, 0);
}

int hugo_playvideo(HUGO_FILE infile, long reslength, char loop_flag, char background, int volume)
{
	glui32 id = 0;
	int i;

	for (i = 0; i < video_count; i++)
	{
		if (!strcmp(videos[i].file, loaded_filename) && !strcmp(videos[i].res, loaded_resname))
		{
			id = i + 1;
			break;
		}
	}
	if (!id)
	{
		unsigned char *buf;
		if (video_count >= MAX_VIDEOS || reslength <= 0 || !(buf = malloc(reslength)))
		{
			fclose(infile);
			return false;
		}
		if (glk_get_buffer_stream(infile, (char *)buf, reslength) != (glui32)reslength)
		{
			free(buf);
			fclose(infile);
			return false;
		}
		strcpy(videos[video_count].file, loaded_filename);
		strcpy(videos[video_count].res, loaded_resname);
		id = ++video_count;
		quengle_video_data(id, buf, reslength);
		free(buf);
	}
	fclose(infile);

	if (volume < 0) volume = 0;
	if (volume > 100) volume = 100;
	quengle_overlay_rect(OVERLAY_VIDEO, id | (loop_flag ? 1u << 16 : 0) | ((glui32)volume << 17));

	if (!background && !loop_flag)
	{
		/* the display starts the clip with the update the first glk_select sends */
		event_t ev;
		glk_request_char_event(mainwin);
		glk_request_timer_events(100);
		for (;;)
		{
			glk_select(&ev);
			if (ev.type == evtype_CharInput)
			{
				hugo_stopvideo();
				break;
			}
			if (ev.type == evtype_Timer && !quengle_video_busy())
			{
				glk_cancel_char_event(mainwin);
				break;
			}
		}
		glk_request_timer_events(0);
	}
	return true;
}

/* a resource out of a resourcefile, without reading the whole file (heres.c FindResource, QUENGLE_RESOURCES):
   the display reads just this resource in place and hands its bytes over; the engine gets a memory stream on
   them. the buffer lives until the next resource is asked for, by which time the engine has closed this one */
unsigned char *quengle_resource_read(const char *file, const char *name, glui32 *len);
static unsigned char *resource_bytes = NULL;

HUGO_FILE quengle_open_resource(char *filename, char *resname, long *length)
{
	glui32 len = 0;
	if (resource_bytes)
	{
		free(resource_bytes);
		resource_bytes = NULL;
	}
	resource_bytes = quengle_resource_read(filename, resname, &len);
	if (!resource_bytes) return NULL;
	*length = len;
	return glk_stream_open_memory((char *)resource_bytes, len, filemode_Read, 0);
}
