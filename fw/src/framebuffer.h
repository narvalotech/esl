#ifndef FRAMEBUFFER_H_
#define FRAMEBUFFER_H_

#include <zephyr/device.h>

/* Initialize the text framebuffer */
int framebuffer_setup(const struct device *dev);

/* Update the display with the current FB contents */
void framebuffer_update(const struct device *dev);

/* Write a single character to the FB */
int framebuffer_write_char(int c);

/* Clears the framebuffer and its internal state.
 *
 * This function can be called before `framebuffer_setup` to allow buffering of
 * characters before the display driver is initialized.
 */
void framebuffer_reset(void);

#endif // FRAMEBUFFER_H_
