// Independent PCM stream: crossfade is baked once; smpl skips the blended intro on wraps.
#include <mmsystem.h>
#include "cvardef.h"
#include <thread>
#include <atomic>
#include <vector>
#include <cstdint>
static std::thread musicThread;
static std::atomic<bool> musicStop{false};
static std::atomic<float> musicGain{0};
static cvar_s* musicEnabled=nullptr;static cvar_s* musicVolume=nullptr;
static void musicTrace(const char* what,unsigned value=0){if(FILE* f=std::fopen("Cremation/diagnostics/music-stream.log","a")){std::fprintf(f,"%s %u\n",what,value);std::fclose(f);}}
static void musicWorker(std::string path){
 FILE* f=std::fopen(path.c_str(),"rb");if(!f){musicTrace("OpenFailed");return;}
 char riff[12];if(std::fread(riff,1,12,f)!=12||std::memcmp(riff,"RIFF",4)||std::memcmp(riff+8,"WAVE",4)){std::fclose(f);return;}
 WAVEFORMATEX format{};unsigned dataAt=0,dataSize=0,loopStart=0;
 for(;;){char id[4];uint32_t size;if(std::fread(id,1,4,f)!=4||std::fread(&size,4,1,f)!=1)break;long at=std::ftell(f);
  if(!std::memcmp(id,"fmt ",4)&&size>=16)std::fread(&format,1,16,f);
  if(!std::memcmp(id,"data",4)){dataAt=at;dataSize=size;}
  if(!std::memcmp(id,"smpl",4)&&size>=60){unsigned info[15];if(std::fread(info,4,15,f)==15)loopStart=info[11];}
  if(std::fseek(f,at+size+(size&1),SEEK_SET))break;
 }
 if(format.wFormatTag!=WAVE_FORMAT_PCM||format.nChannels!=2||format.nSamplesPerSec!=44100||format.wBitsPerSample!=16||!dataAt||loopStart>=dataSize/4){musicTrace("InvalidFormat");std::fclose(f);return;}
 HANDLE event=CreateEventA(nullptr,FALSE,FALSE,nullptr);HWAVEOUT device=nullptr;
 auto result=waveOutOpen(&device,WAVE_MAPPER,&format,reinterpret_cast<DWORD_PTR>(event),0,CALLBACK_EVENT);
 if(result){musicTrace("DeviceFailed",result);CloseHandle(event);std::fclose(f);return;}
 musicTrace("Opened",dataSize/4);const unsigned frames=4096;short buffers[4][frames*2]{};WAVEHDR headers[4]{};unsigned position=0,wraps=0;float gain=0;
 if(GetEnvironmentVariableA("CREMATION_AUDIO_VALIDATE",nullptr,0))position=dataSize/4-22050;
 auto fill=[&](int i){unsigned filled=0;while(filled<frames){if(position>=dataSize/4){position=loopStart;musicTrace("Wrap",++wraps);}unsigned count=std::min(frames-filled,dataSize/4-position);std::fseek(f,dataAt+position*4,SEEK_SET);if(std::fread(buffers[i]+filled*2,4,count,f)!=count){musicStop=true;break;}position+=count;filled+=count;}
  for(unsigned n=0;n<frames;++n){float target=musicGain.load();gain+=(target-gain)*.0002f;for(int c=0;c<2;++c)buffers[i][n*2+c]=short(buffers[i][n*2+c]*gain);}
 };
 for(int i=0;i<4;++i){headers[i].lpData=reinterpret_cast<char*>(buffers[i]);headers[i].dwBufferLength=sizeof(buffers[i]);waveOutPrepareHeader(device,&headers[i],sizeof(WAVEHDR));fill(i);waveOutWrite(device,&headers[i],sizeof(WAVEHDR));}
 unsigned rendered=0;while(!musicStop){WaitForSingleObject(event,50);for(int i=0;i<4&&!musicStop;++i)if(headers[i].dwFlags&WHDR_DONE){rendered+=frames;if(rendered==frames)musicTrace("Playback",rendered);fill(i);waveOutWrite(device,&headers[i],sizeof(WAVEHDR));}}
 waveOutReset(device);for(auto& h:headers)waveOutUnprepareHeader(device,&h,sizeof(h));waveOutClose(device);CloseHandle(event);std::fclose(f);musicTrace("RenderedFrames",rendered);
}
static void musicShutdown(){musicStop=true;if(musicThread.joinable())musicThread.join();}
static void musicUpdate(){
 static DWORD probeStart=0;static bool probeEnded=false;if(GetEnvironmentVariableA("CREMATION_AUDIO_VALIDATE",nullptr,0)&&musicThread.joinable()&&!probeEnded){if(!probeStart)probeStart=GetTickCount();if(GetTickCount()-probeStart>8000){probeEnded=true;eng.pfnClientCmd("disconnect\n");}}
 if(!musicEnabled){musicEnabled=eng.pfnRegisterVariable("cremation_music","1",1);musicVolume=eng.pfnRegisterVariable("cremation_music_volume","0.65",1);}
 const char* level=eng.pfnGetLevelName();bool active=musicEnabled&&musicEnabled->value>0&&level&&*level;
 if(!active){if(musicThread.joinable())musicShutdown();return;}
 float master=eng.pfnGetCvarFloat("volume");musicGain=std::max(0.f,std::min(1.f,musicVolume->value*master));
 if(!musicThread.joinable()){musicStop=false;std::string path=eng.pfnGetGameDirectory();path+="/sound/music/myfuneral_loop.wav";musicThread=std::thread(musicWorker,path);}
}
extern "C" void HUD_Frame(double time){musicUpdate();auto fn=reinterpret_cast<void(*)(double)>(GetProcAddress(donor,"HUD_Frame"));if(fn)fn(time);}
extern "C" void HUD_Shutdown(){musicShutdown();auto fn=reinterpret_cast<void(*)()>(GetProcAddress(donor,"HUD_Shutdown"));if(fn)fn();}
