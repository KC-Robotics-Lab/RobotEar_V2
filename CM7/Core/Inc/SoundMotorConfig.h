#ifndef INC_SOUNDMOTORCONFIG_H_
#define INC_SOUNDMOTORCONFIG_H_

/*
 * Sound azimuth motor build-time selection.
 *
 * Change the two defines below when selecting which azimuth motor driver is
 * used. You may also override them from compiler symbols, for example:
 *   -DSOUND_MOTOR_ENABLE_MX28AR=1
 *   -DSOUND_MOTOR_ENABLE_FEEDBACK360=0
 *
 *   1U = initialize and command the driver
 *   0U = do not initialize or command the driver
 *
 * Supported combinations:
 *   MX28AR  Feedback360  Result
 *   1U      1U           Use both motors at the same time
 *   1U      0U           Use MX28AR only
 *   0U      1U           Use Feedback 360 servo only
 *   0U      0U           Disable azimuth motor output, keep debug target only
 */
#ifndef SOUND_MOTOR_ENABLE_MX28AR
#define SOUND_MOTOR_ENABLE_MX28AR          0U
#endif

#ifndef SOUND_MOTOR_ENABLE_FEEDBACK360
#define SOUND_MOTOR_ENABLE_FEEDBACK360     1U
#endif

/*
 * Backward-compatible aliases.
 * Existing code that used SOUND_MOTOR_USE_* will still compile, but new code
 * should use SOUND_MOTOR_ENABLE_* from this config header.
 */
#ifndef SOUND_MOTOR_USE_MX28AR
#define SOUND_MOTOR_USE_MX28AR             SOUND_MOTOR_ENABLE_MX28AR
#endif

#ifndef SOUND_MOTOR_USE_FEEDBACK360
#define SOUND_MOTOR_USE_FEEDBACK360        SOUND_MOTOR_ENABLE_FEEDBACK360
#endif

#endif /* INC_SOUNDMOTORCONFIG_H_ */
