#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_AUDIO_CMD_LEN 200
#define PLAYER_MAX_URL_LENGTH 200

int em_get_audio_vol()
{
    char command[MAX_AUDIO_CMD_LEN];
    char output[MAX_AUDIO_CMD_LEN];
    int volume = 0;
    sprintf(command, "amixer -D hw:audiocodec cget name='DAC volume'");

    FILE *fp = popen(command, "r");
    if (fp == NULL)
    {
        return 0;
    }

    while (fgets(output, MAX_AUDIO_CMD_LEN, fp) != NULL)
    {
        char *volume_str = strstr(output, " values=");
        if (volume_str != NULL)
        {
            char *volstr = strtok(volume_str, ",");
            while (volstr != NULL)
            {
                volstr = strtok(NULL, ",");
                if (volstr != NULL)
                {
                    volume = atoi(volstr);
                }
            }
            printf("get_audio_vol %d\n", volume);
        }
    }
    pclose(fp);
    return volume;
}

int em_set_audio_vol(int vol)
{
    int ret = 0;
    char cmd[MAX_AUDIO_CMD_LEN];
    memset(cmd, 0, MAX_AUDIO_CMD_LEN);
    sprintf(cmd, "amixer -D hw:audiocodec cset name='DAC volume' 0,%d", vol);
    ret = system(cmd);
    printf("set_audio_vol %s,ret %d\n", cmd, ret);
    return 0;
}

void em_stop_play_audio()
{
    int ret = 0;
    char cmd[200];
    memset(cmd, 0, 200);

    // 停止ffmpeg进程（流媒体播放）
    sprintf(cmd, "killall ffmpeg 2>/dev/null");
    system(cmd);

    // 停止mplayer进程（流媒体播放）
    sprintf(cmd, "killall mplayer 2>/dev/null");
    system(cmd);

    // 停止vlc进程（流媒体播放）
    sprintf(cmd, "killall vlc 2>/dev/null");
    system(cmd);

    // 停止mpg123进程（流媒体播放）
    sprintf(cmd, "killall mpg123 2>/dev/null");
    system(cmd);

    // 停止aplay进程（本地文件播放）
    sprintf(cmd, "killall aplay 2>/dev/null");
    system(cmd);

    printf("stop_play_audio success\n");
}

int em_play_audio(const char *url)
{
    int ret = 0;
    char cmd[PLAYER_MAX_URL_LENGTH + 50];
    if (url == NULL)
    {
        return 0;
    }
    memset(cmd, 0, PLAYER_MAX_URL_LENGTH + 50);
    
    // 检测是否为网络URL
    if (strstr(url, "http://") != NULL || strstr(url, "https://") != NULL)
    {
        // 使用流媒体播放的方式，直接通过媒体播放器播放网络URL
        // 首先尝试使用mplayer
        sprintf(cmd, "mplayer %s &", url);
        ret = system(cmd);
        if (ret != 0) {
            // 如果mplayer不可用，尝试使用vlc
            sprintf(cmd, "vlc --play-and-exit %s &", url);
            ret = system(cmd);
            if (ret != 0) {
                // 如果vlc不可用，尝试使用mpg123
                sprintf(cmd, "mpg123 %s &", url);
                ret = system(cmd);
                if (ret != 0) {
                    // 如果所有播放器都不可用，打印提示信息
                    printf("No media player available. Please install mplayer, vlc, or mpg123.\n");
                }
            }
        }
        printf("Streaming audio: %s, ret %d\n", cmd, ret);
    }
    else
    {
        // 使用aplay播放本地文件
        sprintf(cmd, "aplay %s &", url);
        ret = system(cmd);
        printf("play_local_audio %s,ret %d\n", cmd, ret);
    }
    return 0;
}