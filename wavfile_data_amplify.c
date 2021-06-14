/* wavfile_data_amplify.c
 * 
 * amplify wav audio file data
 * 
 * author: Yangsheng Wang
 * wang_yangsheng@163.com
 * 
 * coding in 2021/6/14
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

struct wav_file_head_info {
	uint32_t chunk_id; /* "RIFF" */
	uint32_t chunk_size; /* 36 + subchunk2_size */
	uint32_t format; /* "WAVE" */

	/* sub-chunk "fmt" */
	uint32_t subchunk1_id; /* "fmt " */
	uint32_t subchunk1_size; /* 16:PCM */
	uint16_t audio_format; /* PCM:1 */
	uint16_t num_channels; /* mono:1, stereo:2 */
	uint32_t sample_rate; /* 8000 16000 ... 44100 48000 */
	uint32_t byte_rate; /* equal sample_rate * num_channels * bits_persample/8 */
	uint16_t block_align; /* equal num_channels * bits_persample/8 */
	uint16_t bits_persample; /* 8bits, 16bits, etc. */

	/* sub-chunk "data" */
	uint32_t subchunk2_id; /* "data" */
	uint32_t subchunk2_size; /* data size */
};
  
int main(int argc, char *argv[])  
{
	int fd_sou;
	int fd_des; 
	struct wav_file_head_info wav;
	char *buf_sou;
	char *buf_des;
	int i;
	signed short int data;
	signed int amplify_data;
	float level;
	int ret;

	if (argc < 4) {
		printf("%s [file.wav] [amplify level(float)] [output.wav]\n", argv[0]);
		return -1;
	}
	level = atof(argv[2]);
	printf("level:%f\n", level);

	fd_sou = open(argv[1], O_RDONLY);
	if (fd_sou < 0) {
		printf("fd_sou:%d\n", fd_sou);
		return -1;
	}

	ret = read(fd_sou, &wav, sizeof(struct wav_file_head_info));
	if (ret < 0) {
		printf("read() ret:%d\n", ret);
		close(fd_sou);
		return -1;
	}

	printf("chunk_id \t%x\n", wav.chunk_id);
	printf("chunk_size \t%d\n", wav.chunk_size);
	printf("format \t\t%x\n", wav.format);
	printf("subchunk1_id \t%x\n", wav.subchunk1_id);
	printf("subchunk1_size \t%d\n", wav.subchunk1_size);
	printf("audio_format \t%d\n", wav.audio_format);
	printf("num_channels \t%d\n", wav.num_channels);
	printf("sample_rate \t%d\n", wav.sample_rate);
	printf("byte_rate \t%d\n", wav.byte_rate);
	printf("block_align \t%d\n", wav.block_align);
	printf("bits_persample \t%d\n", wav.bits_persample);
	printf("subchunk2_id \t%x\n", wav.subchunk2_id);
	printf("Subchunk2_size \t%d\n", wav.subchunk2_size);	
	
	if (wav.bits_persample != 16) {
		printf("only support bits_persample:16\n");
		close(fd_sou);
		return -1;
	}

	buf_sou = (char *)malloc(wav.subchunk2_size);
	if (buf_sou == NULL) {
		printf("malloc() buf_sou, error\n");
		close(fd_sou);
		return -1;
	}

	buf_des = (char *)malloc(wav.subchunk2_size);
	if (buf_des == NULL) {
		printf("malloc() buf_des, error\n");
		close(fd_sou);
		free(buf_sou);
		return -1;
	}

	memset(buf_sou, 0, wav.subchunk2_size);
	memset(buf_des, 0, wav.subchunk2_size);

	ret = read(fd_sou, buf_sou, wav.subchunk2_size);
	if (ret < 0) {
		printf("read() ret:%d\n", ret);
		free(buf_sou);
		free(buf_des);
		close(fd_sou);
		return -1;
	}

	for (i = 0; i < wav.subchunk2_size; i += (wav.bits_persample / 8) ) {
		data = 0;
		data = (unsigned short int)((unsigned char)*(buf_sou + i) | (unsigned short int)((unsigned char)*(buf_sou + i + 1) << 8));
		/* data = *(buf_sou + i) | (*(buf_sou + i + 1) << 8); error */
		amplify_data = data * level;

		if (amplify_data < -0x8000)
			amplify_data = -0x8000;
		else if (amplify_data > 0x7FFF)
			amplify_data = 0x7FFF;

		data = amplify_data;
		*(buf_des + i) = data & 0x00FF;
		*(buf_des + i + 1) = (data & 0xFF00) >> 8;
	}

	fd_des = open(argv[3], O_CREAT | O_WRONLY | O_TRUNC, 0666);
	if (fd_des < 0) {
		printf("open() fd_des, ret:%d\n", ret);
		free(buf_sou);
		free(buf_des);
		close(fd_sou);
		return -1;
	}

	wav.chunk_size = 36 + wav.subchunk2_size;

	ret = write(fd_des, &wav, sizeof(struct wav_file_head_info));
	printf("write() fd_des, ret:%d\n", ret);
	ret = write(fd_des, buf_des, wav.subchunk2_size);
	printf("write() fd_des, ret:%d\n", ret);

	free(buf_sou);
	free(buf_des);

	ret = close(fd_sou);
	printf("close() fd_sou, ret:%d\n", ret);
	ret = close(fd_des);
	printf("close() fd_des, ret:%d\n", ret);

	return 0;  
}
