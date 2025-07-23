#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <functional>

#undef PlaySound

// Forward declaration for raudio types
struct Sound;   // raudio Sound
struct Music;   // raudio Music

// Forward declarations for our asset types
struct SoundAsset;
struct MusicAsset;

// Audio event types for the audio system
enum class AudioEventType {
    SOUND_LOADED,
    SOUND_UNLOADED,
    SOUND_PLAYED,
    SOUND_STOPPED,
    SOUND_PAUSED,
    SOUND_RESUMED,
    MUSIC_LOADED,
    MUSIC_UNLOADED,
    MUSIC_STARTED,
    MUSIC_STOPPED,
    MUSIC_FINISHED,
    AUDIO_ERROR
};

// Audio event structure
struct AudioEvent {
    AudioEventType type;
    std::string soundName;
    std::string message;
    
    AudioEvent(AudioEventType t, const std::string& name = "", const std::string& msg = "")
        : type(t), soundName(name), message(msg) {}
};

// Audio command types for thread-safe operations
enum class AudioCommandType {
    LOAD_SOUND,
    UNLOAD_SOUND,
    PLAY_SOUND,
    STOP_SOUND,
    PAUSE_SOUND,
    RESUME_SOUND,
    SET_SOUND_VOLUME,
    SET_SOUND_PITCH,
    SET_SOUND_PAN,
    LOAD_MUSIC,
    UNLOAD_MUSIC,
    PLAY_MUSIC,
    STOP_MUSIC,
    PAUSE_MUSIC,
    RESUME_MUSIC,
    SET_MUSIC_VOLUME,
    SET_MUSIC_PITCH,
    SET_MUSIC_PAN,
    SET_MASTER_VOLUME,
    STOP_ALL_SOUNDS,
    PAUSE_ALL_SOUNDS,
    RESUME_ALL_SOUNDS
};

// Audio command structure for thread communication
struct AudioCommand {
    AudioCommandType type;
    std::string soundName;
    std::string filePath;
    float value1, value2, value3; // For volume, pitch, pan parameters
    bool boolValue; // For looping
    
    AudioCommand(AudioCommandType t, const std::string& name = "", const std::string& path = "")
        : type(t), soundName(name), filePath(path), value1(1.0f), value2(1.0f), value3(0.5f), boolValue(false) {}
};

// Audio callback type
using AudioEventCallback = std::function<void(const AudioEvent&)>;

// Loaded sound/music information
struct LoadedSound {
    ::Sound* raudiosound;     // Pointer to raudio Sound structure
    std::string filePath;
    float volume;
    float pitch;
    float pan;
    bool isPlaying;
    bool isPaused;
    
    LoadedSound() : raudiosound(nullptr), volume(1.0f), pitch(1.0f), pan(0.5f), isPlaying(false), isPaused(false) {}
    ~LoadedSound(); // Defined in .cpp file
    
    // Disable copy constructor and assignment to prevent double deletion
    LoadedSound(const LoadedSound&) = delete;
    LoadedSound& operator=(const LoadedSound&) = delete;
    
    // Enable move constructor and assignment (defined in .cpp)
    LoadedSound(LoadedSound&& other) noexcept;
    LoadedSound& operator=(LoadedSound&& other) noexcept;
};

struct LoadedMusic {
    ::Music* raudioMusic;     // Pointer to raudio Music structure
    std::string filePath;
    float volume;
    float pitch;
    float pan;
    bool isPlaying;
    bool isPaused;
    bool isLooping;
    
    LoadedMusic() : raudioMusic(nullptr), volume(1.0f), pitch(1.0f), pan(0.5f), isPlaying(false), isPaused(false), isLooping(false) {}
    ~LoadedMusic(); // Defined in .cpp file
    
    // Disable copy constructor and assignment to prevent double deletion
    LoadedMusic(const LoadedMusic&) = delete;
    LoadedMusic& operator=(const LoadedMusic&) = delete;
    
    // Enable move constructor and assignment (defined in .cpp)
    LoadedMusic(LoadedMusic&& other) noexcept;
    LoadedMusic& operator=(LoadedMusic&& other) noexcept;
};

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    // Initialization and cleanup
    bool Initialize();
    void Shutdown();
    bool IsInitialized() const { return m_Initialized; }
    
    // Sound management
    bool LoadSound(const std::string& soundName, const std::string& filePath);
    void UnloadSound(const std::string& soundName);
    bool IsSoundLoaded(const std::string& soundName) const;
    
    // Sound playback
    void PlayAudio(const std::string& soundName);
    void StopAudio(const std::string& soundName);
    void PauseAudio(const std::string& soundName);
    void ResumeAudio(const std::string& soundName);
    bool IsAudioPlaying(const std::string& soundName) const;
    bool IsAudioPaused(const std::string& soundName) const;
    
    // Sound control
    void SetSoundVolume(const std::string& soundName, float volume);
    void SetSoundPitch(const std::string& soundName, float pitch);
    void SetSoundPan(const std::string& soundName, float pan);
    
    // Music management
    bool LoadMusic(const std::string& musicName, const std::string& filePath);
    void UnloadMusic(const std::string& musicName);
    bool IsMusicLoaded(const std::string& musicName) const;
    
    // Music playback
    void PlayMusic(const std::string& musicName, bool loop = true);
    void StopMusic(const std::string& musicName);
    void PauseMusic(const std::string& musicName);
    void ResumeMusic(const std::string& musicName);
    bool IsMusicPlaying(const std::string& musicName) const;
    bool IsMusicPaused(const std::string& musicName) const;
    
    // Music control
    void SetMusicVolume(const std::string& musicName, float volume);
    void SetMusicPitch(const std::string& musicName, float pitch);
    void SetMusicPan(const std::string& musicName, float pan);
    void SeekMusic(const std::string& musicName, float position);
    float GetMusicTimeLength(const std::string& musicName) const;
    float GetMusicTimePlayed(const std::string& musicName) const;
    
    // Global audio control
    void SetMasterVolume(float volume);
    float GetMasterVolume() const;
    void StopAllSounds();
    void PauseAllSounds();
    void ResumeAllSounds();
    void StopAllMusic();
    void PauseAllMusic();
    void ResumeAllMusic();
    
    // Batch operations for efficiency
    void LoadSoundBatch(const std::vector<SoundAsset>& sounds);
    void LoadMusicBatch(const std::vector<MusicAsset>& music);
    
    // Event handling
    void SetEventCallback(AudioEventCallback callback) { m_EventCallback = callback; }
    
    // Update function (call every frame to process events)
    void Update();
    
    // Statistics and info
    size_t GetLoadedSoundCount() const;
    size_t GetLoadedMusicCount() const;
    std::vector<std::string> GetLoadedSoundNames() const;
    std::vector<std::string> GetLoadedMusicNames() const;
    
    // Error handling
    static std::string GetLastError() { return s_LastError; }
    
private:
    // Internal state
    std::atomic<bool> m_Initialized;
    std::atomic<float> m_MasterVolume;
    
    // Audio resources (protected by mutex)
    std::unordered_map<std::string, LoadedSound> m_LoadedSounds;
    std::unordered_map<std::string, LoadedMusic> m_LoadedMusic;
    mutable std::mutex m_AudioResourcesMutex;
    
    // Threading
    std::thread m_AudioThread;
    std::atomic<bool> m_ThreadRunning;
    std::mutex m_CommandQueueMutex;
    std::condition_variable m_ThreadCondition;
    std::queue<AudioCommand> m_CommandQueue;
    
    // Event handling
    AudioEventCallback m_EventCallback;
    std::queue<AudioEvent> m_EventQueue;
    std::mutex m_EventQueueMutex;
    
    // Error handling
    static std::string s_LastError;
    
    // Internal methods
    void AudioThreadFunction();
    void ProcessCommand(const AudioCommand& command);
    void QueueCommand(const AudioCommand& command);
    void QueueEvent(const AudioEvent& event);
    void SetError(const std::string& error);
    void UpdateMusicStreams(); // Update all active music streams
    void CleanupFinishedSounds(); // Clean up sounds that have finished playing
    
    // Command processors
    void ProcessLoadSound(const AudioCommand& cmd);
    void ProcessUnloadSound(const AudioCommand& cmd);
    void ProcessPlaySound(const AudioCommand& cmd);
    void ProcessStopSound(const AudioCommand& cmd);
    void ProcessPauseSound(const AudioCommand& cmd);
    void ProcessResumeSound(const AudioCommand& cmd);
    void ProcessSetSoundVolume(const AudioCommand& cmd);
    void ProcessSetSoundPitch(const AudioCommand& cmd);
    void ProcessSetSoundPan(const AudioCommand& cmd);
    
    void ProcessLoadMusic(const AudioCommand& cmd);
    void ProcessUnloadMusic(const AudioCommand& cmd);
    void ProcessPlayMusic(const AudioCommand& cmd);
    void ProcessStopMusic(const AudioCommand& cmd);
    void ProcessPauseMusic(const AudioCommand& cmd);
    void ProcessResumeMusic(const AudioCommand& cmd);
    void ProcessSetMusicVolume(const AudioCommand& cmd);
    void ProcessSetMusicPitch(const AudioCommand& cmd);
    void ProcessSetMusicPan(const AudioCommand& cmd);
    
    void ProcessSetMasterVolume(const AudioCommand& cmd);
    void ProcessStopAllSounds(const AudioCommand& cmd);
    void ProcessPauseAllSounds(const AudioCommand& cmd);
    void ProcessResumeAllSounds(const AudioCommand& cmd);
};

// Global audio manager instance (singleton pattern like NetworkManager)
namespace Audio {
    AudioManager& GetManager();
    bool Initialize();
    void Shutdown();
    
    // Convenience functions
    inline bool LoadSound(const std::string& name, const std::string& path) {
        return GetManager().LoadSound(name, path);
    }
    
    inline void PlaySound(const std::string& name) {
        GetManager().PlayAudio(name);
    }
    
    inline void StopSound(const std::string& name) {
        GetManager().StopAudio(name);
    }
    
    inline bool LoadMusic(const std::string& name, const std::string& path) {
        return GetManager().LoadMusic(name, path);
    }
    
    inline void PlayMusic(const std::string& name, bool loop = true) {
        GetManager().PlayMusic(name, loop);
    }
    
    inline void StopMusic(const std::string& name) {
        GetManager().StopMusic(name);
    }
    
    inline void SetMasterVolume(float volume) {
        GetManager().SetMasterVolume(volume);
    }
    
    inline void Update() {
        GetManager().Update();
    }
}