#include "AudioManager.h"
#include "Sound.h"
#include <engine/utils/Logger.h>

// Include raudio
extern "C" {
    #include <raudio.h>
}

#include <algorithm>
#include <chrono>

// Static error storage
std::string AudioManager::s_LastError = "";

// LoadedSound destructor implementation
LoadedSound::~LoadedSound() {
    if (raudiosound) {
        // First stop the sound before unloading to prevent callbacks
        if (::IsSoundPlaying(*raudiosound)) {
            ::StopSound(*raudiosound);
        }
        
        // Now safe to unload
        if (::IsSoundReady(*raudiosound)) {
            ::UnloadSound(*raudiosound);
        }
        
        delete raudiosound;
        raudiosound = nullptr; // Prevent potential double-free
    }
}

// LoadedMusic destructor implementation  
LoadedMusic::~LoadedMusic() {
    if (raudioMusic) {
        // First stop the music stream to prevent callbacks
        if (::IsMusicStreamPlaying(*raudioMusic)) {
            ::StopMusicStream(*raudioMusic);
        }
        
        // Now safe to unload
        if (::IsMusicReady(*raudioMusic)) {
            ::UnloadMusicStream(*raudioMusic);
        }
        
        delete raudioMusic;
        raudioMusic = nullptr; // Prevent potential double-free
    }
}

// LoadedSound move constructor and assignment
LoadedSound::LoadedSound(LoadedSound&& other) noexcept 
    : raudiosound(other.raudiosound), filePath(std::move(other.filePath)),
      volume(other.volume), pitch(other.pitch), pan(other.pan),
      isPlaying(other.isPlaying), isPaused(other.isPaused) {
    other.raudiosound = nullptr;
}

LoadedSound& LoadedSound::operator=(LoadedSound&& other) noexcept {
    if (this != &other) {
        // Properly cleanup existing resource
        if (raudiosound) {
            // Stop the sound first to prevent callbacks
            if (::IsSoundPlaying(*raudiosound)) {
                ::StopSound(*raudiosound);
            }
            
            if (::IsSoundReady(*raudiosound)) {
                ::UnloadSound(*raudiosound);
            }
            delete raudiosound;
            raudiosound = nullptr;
        }
        
        // Move resources
        raudiosound = other.raudiosound;
        filePath = std::move(other.filePath);
        volume = other.volume;
        pitch = other.pitch;
        pan = other.pan;
        isPlaying = other.isPlaying;
        isPaused = other.isPaused;
        
        // Clear the source pointer to prevent double deletion
        other.raudiosound = nullptr;
        other.isPlaying = false;
        other.isPaused = false;
    }
    return *this;
}

// LoadedMusic move constructor and assignment
LoadedMusic::LoadedMusic(LoadedMusic&& other) noexcept 
    : raudioMusic(other.raudioMusic), filePath(std::move(other.filePath)),
      volume(other.volume), pitch(other.pitch), pan(other.pan),
      isPlaying(other.isPlaying), isPaused(other.isPaused), isLooping(other.isLooping) {
    other.raudioMusic = nullptr;
}

LoadedMusic& LoadedMusic::operator=(LoadedMusic&& other) noexcept {
    if (this != &other) {
        // Properly cleanup existing resource
        if (raudioMusic) {
            // Stop the music stream first to prevent callbacks
            if (::IsMusicStreamPlaying(*raudioMusic)) {
                ::StopMusicStream(*raudioMusic);
            }
            
            if (::IsMusicReady(*raudioMusic)) {
                ::UnloadMusicStream(*raudioMusic);
            }
            delete raudioMusic;
            raudioMusic = nullptr;
        }
        
        // Move resources
        raudioMusic = other.raudioMusic;
        filePath = std::move(other.filePath);
        volume = other.volume;
        pitch = other.pitch;
        pan = other.pan;
        isPlaying = other.isPlaying;
        isPaused = other.isPaused;
        isLooping = other.isLooping;
        
        // Clear the source pointer to prevent double deletion
        other.raudioMusic = nullptr;
        other.isPlaying = false;
        other.isPaused = false;
        other.isLooping = false;
    }
    return *this;
}

AudioManager::AudioManager()
    : m_Initialized(false)
    , m_MasterVolume(1.0f)
    , m_ThreadRunning(false)
    , m_EventCallback(nullptr)
{
}

AudioManager::~AudioManager() {
    Shutdown();
}

bool AudioManager::Initialize() {
    if (m_Initialized.load()) {
        return true;
    }
    
    // Initialize raudio
    ::InitAudioDevice();
    
    if (!::IsAudioDeviceReady()) {
        SetError("Failed to initialize audio device");
        return false;
    }
    
    // Set initial master volume
    ::SetMasterVolume(m_MasterVolume.load());
    
    // Start audio thread
    m_ThreadRunning = true;
    m_AudioThread = std::thread(&AudioManager::AudioThreadFunction, this);
    
    m_Initialized = true;
    
    Logger::Info("AudioManager initialized successfully");
    return true;
}

void AudioManager::Shutdown() {
    if (!m_Initialized.load()) {
        return;
    }
    
    Logger::Info("Clearing event callback");
    // First, clear the event callback to prevent any further callbacks
    {
        std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
        m_EventCallback = nullptr;
    }
    
    Logger::Info("Stopping all active sounds and music");
    // Stop all active sounds and music to prevent callbacks
    {
        std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
        
        // Stop all active sounds
        for (auto& pair : m_LoadedSounds) {
            if (pair.second.raudiosound && pair.second.isPlaying) {
                if (::IsSoundPlaying(*pair.second.raudiosound)) {
                    ::StopSound(*pair.second.raudiosound);
                }
                pair.second.isPlaying = false;
                pair.second.isPaused = false;
            }
        }
        
        // Stop all active music streams
        for (auto& pair : m_LoadedMusic) {
            if (pair.second.raudioMusic && pair.second.isPlaying) {
                if (::IsMusicStreamPlaying(*pair.second.raudioMusic)) {
                    ::StopMusicStream(*pair.second.raudioMusic);
                }
                pair.second.isPlaying = false;
                pair.second.isPaused = false;
            }
        }
    }
    
    Logger::Info("Clearing command queue");
    // Give a brief pause to let any audio callbacks complete
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    Logger::Info("Clearing event queue");
    // Now stop the audio thread
    if (m_ThreadRunning.load()) {
        // Clear all pending commands
        {
            std::lock_guard<std::mutex> lock(m_CommandQueueMutex);
            std::queue<AudioCommand> empty;
            std::swap(m_CommandQueue, empty);
        }
        
        // Clear all pending events
        {
            std::lock_guard<std::mutex> lock(m_EventQueueMutex);
            std::queue<AudioEvent> empty;
            std::swap(m_EventQueue, empty);
        }
        
        Logger::Info("Signaling thread to stop and join");
        // Signal thread to stop and join
        m_ThreadRunning = false;
        m_ThreadCondition.notify_all();
        if (m_AudioThread.joinable()) {
            m_AudioThread.join();
        }
        
        Logger::Info("Sleeping for 10ms");
        // Additional safety measure - ensure thread has fully stopped
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    Logger::Info("Clearing audio resources");
    // Finally, clean up all resources
    {
        std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
        
        Logger::Info("Clearing event queue again");
        // Clear event queue again (just in case)
        while (!m_EventQueue.empty()) {
            m_EventQueue.pop();
        }
        
        // Unload all sounds
        Logger::Info("Unloading all sounds");
        for (auto& pair : m_LoadedSounds) {
            if (pair.second.raudiosound) {
                Logger::Info("  Unloading sound: " + pair.first + ", ptr: " + std::to_string(reinterpret_cast<uintptr_t>(pair.second.raudiosound)));
                if (::IsSoundReady(*pair.second.raudiosound)) {
                    ::UnloadSound(*pair.second.raudiosound);
                    Logger::Info("    Unloaded sound: " + pair.first);
                } else {
                    Logger::Warn<AudioManager>("    Sound not ready: " + pair.first, this);
                }
                delete pair.second.raudiosound;
                pair.second.raudiosound = nullptr;
            } else {
                Logger::Warn<AudioManager>("  Sound pointer already null: " + pair.first, this);
            }
        }
        m_LoadedSounds.clear();
        
        // Unload all music
        Logger::Info("Unloading all music");
        for (auto& pair : m_LoadedMusic) {
            if (pair.second.raudioMusic) {
                Logger::Info("  Unloading music: " + pair.first + ", ptr: " + std::to_string(reinterpret_cast<uintptr_t>(pair.second.raudioMusic)));
                if (::IsMusicReady(*pair.second.raudioMusic)) {
                    ::UnloadMusicStream(*pair.second.raudioMusic);
                    Logger::Info("    Unloaded music: " + pair.first);
                } else {
                    Logger::Warn<AudioManager>("    Music not ready: " + pair.first, this);
                }
                delete pair.second.raudioMusic;
                pair.second.raudioMusic = nullptr;
            } else {
                Logger::Warn<AudioManager>("  Music pointer already null: " + pair.first, this);
            }
        }
        m_LoadedMusic.clear();
    }
    
    Logger::Info("Closing audio device");
    // Close audio device
    ::CloseAudioDevice();
    
    Logger::Info("Setting initialized to false");
    m_Initialized = false;
    
    Logger::Info("AudioManager shut down");
}

void AudioManager::AudioThreadFunction() {
    Logger::Info("Audio thread started");
    
    const auto frameTime = std::chrono::milliseconds(16); // ~60 FPS for smooth music streaming
    auto lastTime = std::chrono::steady_clock::now();
    
    while (m_ThreadRunning.load()) {
        try {
            auto currentTime = std::chrono::steady_clock::now();
            
            // Process commands
            std::queue<AudioCommand> commandsToProcess;
            {
                std::lock_guard<std::mutex> lock(m_CommandQueueMutex);
                commandsToProcess = std::move(m_CommandQueue);
                m_CommandQueue = std::queue<AudioCommand>();
            }
            
            while (!commandsToProcess.empty()) {
                try {
                    ProcessCommand(commandsToProcess.front());
                }
                catch (const std::exception& e) {
                    Logger::Error<AudioManager>("Exception processing audio command: " + std::string(e.what()), this);
                }
                catch (...) {
                    Logger::Error<AudioManager>("Unknown error processing audio command", this);
                }
                commandsToProcess.pop();
            }
            
            // Update music streams (critical for streaming audio)
            try {
                UpdateMusicStreams();
            }
            catch (const std::exception& e) {
                Logger::Error<AudioManager>("Exception updating music streams: " + std::string(e.what()), this);
            }
            catch (...) {
                Logger::Error<AudioManager>("Unknown error updating music streams", this);
            }
            
            // Clean up finished sounds periodically
            if (currentTime - lastTime >= std::chrono::seconds(1)) {
                try {
                    CleanupFinishedSounds();
                }
                catch (const std::exception& e) {
                    Logger::Error<AudioManager>("Exception cleaning up sounds: " + std::string(e.what()), this);
                }
                catch (...) {
                    Logger::Error<AudioManager>("Unknown error cleaning up sounds", this);
                }
                lastTime = currentTime;
            }
            
            // Wait for next frame
            std::unique_lock<std::mutex> lock(m_CommandQueueMutex);
            m_ThreadCondition.wait_for(lock, frameTime);
        }
        catch (const std::exception& e) {
            Logger::Error<AudioManager>("Critical error in audio thread: " + std::string(e.what()), this);
            // Brief pause to avoid CPU hammering if there's a persistent error
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        catch (...) {
            Logger::Error<AudioManager>("Unknown critical error in audio thread", this);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    Logger::Info("Audio thread stopped");
}

void AudioManager::ProcessCommand(const AudioCommand& command) {
    switch (command.type) {
        case AudioCommandType::LOAD_SOUND:
            ProcessLoadSound(command);
            break;
        case AudioCommandType::UNLOAD_SOUND:
            ProcessUnloadSound(command);
            break;
        case AudioCommandType::PLAY_SOUND:
            ProcessPlaySound(command);
            break;
        case AudioCommandType::STOP_SOUND:
            ProcessStopSound(command);
            break;
        case AudioCommandType::PAUSE_SOUND:
            ProcessPauseSound(command);
            break;
        case AudioCommandType::RESUME_SOUND:
            ProcessResumeSound(command);
            break;
        case AudioCommandType::SET_SOUND_VOLUME:
            ProcessSetSoundVolume(command);
            break;
        case AudioCommandType::SET_SOUND_PITCH:
            ProcessSetSoundPitch(command);
            break;
        case AudioCommandType::SET_SOUND_PAN:
            ProcessSetSoundPan(command);
            break;
        case AudioCommandType::LOAD_MUSIC:
            ProcessLoadMusic(command);
            break;
        case AudioCommandType::UNLOAD_MUSIC:
            ProcessUnloadMusic(command);
            break;
        case AudioCommandType::PLAY_MUSIC:
            ProcessPlayMusic(command);
            break;
        case AudioCommandType::STOP_MUSIC:
            ProcessStopMusic(command);
            break;
        case AudioCommandType::PAUSE_MUSIC:
            ProcessPauseMusic(command);
            break;
        case AudioCommandType::RESUME_MUSIC:
            ProcessResumeMusic(command);
            break;
        case AudioCommandType::SET_MUSIC_VOLUME:
            ProcessSetMusicVolume(command);
            break;
        case AudioCommandType::SET_MUSIC_PITCH:
            ProcessSetMusicPitch(command);
            break;
        case AudioCommandType::SET_MUSIC_PAN:
            ProcessSetMusicPan(command);
            break;
        case AudioCommandType::SET_MASTER_VOLUME:
            ProcessSetMasterVolume(command);
            break;
        case AudioCommandType::STOP_ALL_SOUNDS:
            ProcessStopAllSounds(command);
            break;
        case AudioCommandType::PAUSE_ALL_SOUNDS:
            ProcessPauseAllSounds(command);
            break;
        case AudioCommandType::RESUME_ALL_SOUNDS:
            ProcessResumeAllSounds(command);
            break;
    }
}

// Sound management implementations
void AudioManager::ProcessLoadSound(const AudioCommand& cmd) {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    if (m_LoadedSounds.find(cmd.soundName) != m_LoadedSounds.end()) {
        Logger::Warn<AudioManager>("Sound '" + cmd.soundName + "' already loaded", this);
        return;
    }
    
    ::Sound raudioSound = ::LoadSound(cmd.filePath.c_str());
    
    if (!::IsSoundReady(raudioSound)) {
        SetError("Failed to load sound: " + cmd.filePath);
        QueueEvent(AudioEvent(AudioEventType::AUDIO_ERROR, cmd.soundName, 
                             "Failed to load sound: " + cmd.filePath));
        return;
    }
    
    LoadedSound loadedSound;
    loadedSound.raudiosound = new ::Sound(raudioSound);  // Allocate and copy
    loadedSound.filePath = cmd.filePath;
    loadedSound.volume = cmd.value1;
    loadedSound.pitch = cmd.value2;
    loadedSound.pan = cmd.value3;
    
    // Apply initial settings
    ::SetSoundVolume(*loadedSound.raudiosound, loadedSound.volume);
    ::SetSoundPitch(*loadedSound.raudiosound, loadedSound.pitch);
    ::SetSoundPan(*loadedSound.raudiosound, loadedSound.pan);
    
    m_LoadedSounds[cmd.soundName] = std::move(loadedSound);
    
    QueueEvent(AudioEvent(AudioEventType::SOUND_LOADED, cmd.soundName, 
                         "Sound loaded: " + cmd.filePath));
    
    Logger::Info("Sound loaded: " + cmd.soundName + " from " + cmd.filePath);
}

void AudioManager::ProcessUnloadSound(const AudioCommand& cmd) {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    auto it = m_LoadedSounds.find(cmd.soundName);
    if (it == m_LoadedSounds.end()) {
        Logger::Warn<AudioManager>("Sound '" + cmd.soundName + "' not found for unloading", this);
        return;
    }
    
    if (it->second.raudiosound && ::IsSoundReady(*it->second.raudiosound)) {
        ::UnloadSound(*it->second.raudiosound);
    }
    
    m_LoadedSounds.erase(it);
    
    QueueEvent(AudioEvent(AudioEventType::SOUND_UNLOADED, cmd.soundName));
    
    Logger::Info("Sound unloaded: " + cmd.soundName);
}

void AudioManager::ProcessPlaySound(const AudioCommand& cmd) {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    auto it = m_LoadedSounds.find(cmd.soundName);
    if (it == m_LoadedSounds.end()) {
        Logger::Warn<AudioManager>("Sound '" + cmd.soundName + "' not found for playing", this);
        return;
    }
    
    if (it->second.raudiosound) {
        ::PlaySound(*it->second.raudiosound);
        it->second.isPlaying = true;
        it->second.isPaused = false;
        
        QueueEvent(AudioEvent(AudioEventType::SOUND_PLAYED, cmd.soundName));
    }
}

void AudioManager::ProcessStopSound(const AudioCommand& cmd) {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    auto it = m_LoadedSounds.find(cmd.soundName);
    if (it == m_LoadedSounds.end()) {
        return;
    }
    
    if (it->second.raudiosound) {
        ::StopSound(*it->second.raudiosound);
        it->second.isPlaying = false;
        it->second.isPaused = false;
        
        QueueEvent(AudioEvent(AudioEventType::SOUND_STOPPED, cmd.soundName));
    }
}

void AudioManager::ProcessPauseSound(const AudioCommand& cmd) {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    auto it = m_LoadedSounds.find(cmd.soundName);
    if (it == m_LoadedSounds.end()) {
        return;
    }
    
    if (it->second.raudiosound) {
        ::PauseSound(*it->second.raudiosound);
        it->second.isPaused = true;
        
        QueueEvent(AudioEvent(AudioEventType::SOUND_PAUSED, cmd.soundName));
    }
}

void AudioManager::ProcessResumeSound(const AudioCommand& cmd) {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    auto it = m_LoadedSounds.find(cmd.soundName);
    if (it == m_LoadedSounds.end()) {
        return;
    }
    
    if (it->second.raudiosound) {
        ::ResumeSound(*it->second.raudiosound);
        it->second.isPaused = false;
        
        QueueEvent(AudioEvent(AudioEventType::SOUND_RESUMED, cmd.soundName));
    }
}

void AudioManager::ProcessSetSoundVolume(const AudioCommand& cmd) {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    auto it = m_LoadedSounds.find(cmd.soundName);
    if (it == m_LoadedSounds.end()) {
        return;
    }
    
    if (it->second.raudiosound) {
        it->second.volume = cmd.value1;
        ::SetSoundVolume(*it->second.raudiosound, cmd.value1);
    }
}

void AudioManager::ProcessSetSoundPitch(const AudioCommand& cmd) {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    auto it = m_LoadedSounds.find(cmd.soundName);
    if (it == m_LoadedSounds.end()) {
        return;
    }
    
    if (it->second.raudiosound) {
        it->second.pitch = cmd.value1;
        ::SetSoundPitch(*it->second.raudiosound, cmd.value1);
    }
}

void AudioManager::ProcessSetSoundPan(const AudioCommand& cmd) {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    auto it = m_LoadedSounds.find(cmd.soundName);
    if (it == m_LoadedSounds.end()) {
        return;
    }
    
    if (it->second.raudiosound) {
        it->second.pan = cmd.value1;
        ::SetSoundPan(*it->second.raudiosound, cmd.value1);
    }
}

// Music management implementations
void AudioManager::ProcessLoadMusic(const AudioCommand& cmd) {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    if (m_LoadedMusic.find(cmd.soundName) != m_LoadedMusic.end()) {
        Logger::Warn<AudioManager>("Music '" + cmd.soundName + "' already loaded", this);
        return;
    }
    
        ::Music raudioMusic = ::LoadMusicStream(cmd.filePath.c_str());
    
    if (!::IsMusicReady(raudioMusic)) {
        SetError("Failed to load music: " + cmd.filePath);
        QueueEvent(AudioEvent(AudioEventType::AUDIO_ERROR, cmd.soundName, 
                             "Failed to load music: " + cmd.filePath));
        return;
    }
    
    LoadedMusic loadedMusic;
    loadedMusic.raudioMusic = new ::Music(raudioMusic);  // Allocate and copy
    loadedMusic.filePath = cmd.filePath;
    loadedMusic.volume = cmd.value1;
    loadedMusic.pitch = cmd.value2;
    loadedMusic.pan = cmd.value3;
    loadedMusic.isLooping = cmd.boolValue;
    
    // Apply initial settings
    ::SetMusicVolume(*loadedMusic.raudioMusic, loadedMusic.volume);
    ::SetMusicPitch(*loadedMusic.raudioMusic, loadedMusic.pitch);
    ::SetMusicPan(*loadedMusic.raudioMusic, loadedMusic.pan);
    
    m_LoadedMusic[cmd.soundName] = std::move(loadedMusic);
    
    QueueEvent(AudioEvent(AudioEventType::MUSIC_LOADED, cmd.soundName, 
                          "Music loaded: " + cmd.filePath));
    
    Logger::Info("Music loaded: " + cmd.soundName + " from " + cmd.filePath);
}

void AudioManager::ProcessUnloadMusic(const AudioCommand& cmd) {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    auto it = m_LoadedMusic.find(cmd.soundName);
    if (it == m_LoadedMusic.end()) {
        Logger::Warn<AudioManager>("Music '" + cmd.soundName + "' not found for unloading", this);
        return;
    }
    
    if (it->second.raudioMusic && ::IsMusicReady(*it->second.raudioMusic)) {
        ::UnloadMusicStream(*it->second.raudioMusic);
    }
    
    m_LoadedMusic.erase(it);
    
    QueueEvent(AudioEvent(AudioEventType::MUSIC_UNLOADED, cmd.soundName));
    
    Logger::Info("Music unloaded: " + cmd.soundName);
}

void AudioManager::ProcessPlayMusic(const AudioCommand& cmd) {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    auto it = m_LoadedMusic.find(cmd.soundName);
    if (it == m_LoadedMusic.end()) {
        Logger::Warn<AudioManager>("Music '" + cmd.soundName + "' not found for playing", this);
        return;
    }
    
    if (it->second.raudioMusic) {
        it->second.raudioMusic->looping = cmd.boolValue;
        ::PlayMusicStream(*it->second.raudioMusic);
        it->second.isPlaying = true;
        it->second.isPaused = false;
        it->second.isLooping = cmd.boolValue;
        
        QueueEvent(AudioEvent(AudioEventType::MUSIC_STARTED, cmd.soundName));
    }
}

void AudioManager::ProcessStopMusic(const AudioCommand& cmd) {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    auto it = m_LoadedMusic.find(cmd.soundName);
    if (it == m_LoadedMusic.end()) {
        return;
    }
    
    if (it->second.raudioMusic) {
        ::StopMusicStream(*it->second.raudioMusic);
        it->second.isPlaying = false;
        it->second.isPaused = false;
        
        QueueEvent(AudioEvent(AudioEventType::MUSIC_STOPPED, cmd.soundName));
    }
}

void AudioManager::ProcessPauseMusic(const AudioCommand& cmd) {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    auto it = m_LoadedMusic.find(cmd.soundName);
    if (it == m_LoadedMusic.end()) {
        return;
    }
    
    if (it->second.raudioMusic) {
        ::PauseMusicStream(*it->second.raudioMusic);
        it->second.isPaused = true;
    }
}

void AudioManager::ProcessResumeMusic(const AudioCommand& cmd) {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    auto it = m_LoadedMusic.find(cmd.soundName);
    if (it == m_LoadedMusic.end()) {
        return;
    }
    
    if (it->second.raudioMusic) {
        ::ResumeMusicStream(*it->second.raudioMusic);
        it->second.isPaused = false;
    }
}

void AudioManager::ProcessSetMusicVolume(const AudioCommand& cmd) {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    auto it = m_LoadedMusic.find(cmd.soundName);
    if (it == m_LoadedMusic.end()) {
        return;
    }
    
    if (it->second.raudioMusic) {
        it->second.volume = cmd.value1;
        ::SetMusicVolume(*it->second.raudioMusic, cmd.value1);
    }
}

void AudioManager::ProcessSetMusicPitch(const AudioCommand& cmd) {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    auto it = m_LoadedMusic.find(cmd.soundName);
    if (it == m_LoadedMusic.end()) {
        return;
    }
    
    if (it->second.raudioMusic) {
        it->second.pitch = cmd.value1;
        ::SetMusicPitch(*it->second.raudioMusic, cmd.value1);
    }
}

void AudioManager::ProcessSetMusicPan(const AudioCommand& cmd) {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    auto it = m_LoadedMusic.find(cmd.soundName);
    if (it == m_LoadedMusic.end()) {
        return;
    }
    
    if (it->second.raudioMusic) {
        it->second.pan = cmd.value1;
        ::SetMusicPan(*it->second.raudioMusic, cmd.value1);
    }
}

// Global control implementations
void AudioManager::ProcessSetMasterVolume(const AudioCommand& cmd) {
    m_MasterVolume = cmd.value1;
    ::SetMasterVolume(cmd.value1);
}

void AudioManager::ProcessStopAllSounds(const AudioCommand& cmd) {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    for (auto& pair : m_LoadedSounds) {
        if (pair.second.raudiosound) {
            ::StopSound(*pair.second.raudiosound);
            pair.second.isPlaying = false;
            pair.second.isPaused = false;
        }
    }
}

void AudioManager::ProcessPauseAllSounds(const AudioCommand& cmd) {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    for (auto& pair : m_LoadedSounds) {
        if (pair.second.isPlaying && !pair.second.isPaused && pair.second.raudiosound) {
            ::PauseSound(*pair.second.raudiosound);
            pair.second.isPaused = true;
        }
    }
}

void AudioManager::ProcessResumeAllSounds(const AudioCommand& cmd) {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    for (auto& pair : m_LoadedSounds) {
        if (pair.second.isPaused && pair.second.raudiosound) {
            ::ResumeSound(*pair.second.raudiosound);
            pair.second.isPaused = false;
        }
    }
}

// Utility methods
void AudioManager::UpdateMusicStreams() {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    for (auto& pair : m_LoadedMusic) {
        if (pair.second.isPlaying && pair.second.raudioMusic) {
            ::UpdateMusicStream(*pair.second.raudioMusic);
            
            // Check if music has finished (for non-looping music)
            if (!::IsMusicStreamPlaying(*pair.second.raudioMusic) && !pair.second.isLooping) {
                pair.second.isPlaying = false;
                QueueEvent(AudioEvent(AudioEventType::MUSIC_FINISHED, pair.first));
            }
        }
    }
}

void AudioManager::CleanupFinishedSounds() {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    for (auto& pair : m_LoadedSounds) {
        if (pair.second.isPlaying && pair.second.raudiosound && !::IsSoundPlaying(*pair.second.raudiosound)) {
            pair.second.isPlaying = false;
            pair.second.isPaused = false;
            
            // Send a SOUND_STOPPED event when a sound finishes playing
            QueueEvent(AudioEvent(AudioEventType::SOUND_STOPPED, pair.first));
            Logger::Debug<AudioManager>("Sound finished playing: " + pair.first, this);
        }
    }
}

void AudioManager::QueueCommand(const AudioCommand& command) {
    std::lock_guard<std::mutex> lock(m_CommandQueueMutex);
    m_CommandQueue.push(command);
    m_ThreadCondition.notify_one();
}

void AudioManager::QueueEvent(const AudioEvent& event) {
    std::lock_guard<std::mutex> lock(m_EventQueueMutex);
    m_EventQueue.push(event);
}

void AudioManager::SetError(const std::string& error) {
    s_LastError = error;
    Logger::Error<AudioManager>(error, this);
}

// Public API implementations
bool AudioManager::LoadSound(const std::string& soundName, const std::string& filePath) {
    if (!m_Initialized.load()) {
        SetError("AudioManager not initialized");
        return false;
    }
    
    AudioCommand cmd(AudioCommandType::LOAD_SOUND, soundName, filePath);
    QueueCommand(cmd);
    return true;
}

void AudioManager::UnloadSound(const std::string& soundName) {
    if (!m_Initialized.load()) return;
    
    AudioCommand cmd(AudioCommandType::UNLOAD_SOUND, soundName);
    QueueCommand(cmd);
}

void AudioManager::PlayAudio(const std::string& soundName) {
    if (!m_Initialized.load()) return;
    
    AudioCommand cmd(AudioCommandType::PLAY_SOUND, soundName);
    QueueCommand(cmd);
}

void AudioManager::StopAudio(const std::string& soundName) {
    if (!m_Initialized.load()) return;
    
    AudioCommand cmd(AudioCommandType::STOP_SOUND, soundName);
    QueueCommand(cmd);
}

void AudioManager::PauseAudio(const std::string& soundName) {
    if (!m_Initialized.load()) return;
    
    AudioCommand cmd(AudioCommandType::PAUSE_SOUND, soundName);
    QueueCommand(cmd);
}

void AudioManager::ResumeAudio(const std::string& soundName) {
    if (!m_Initialized.load()) return;
    
    AudioCommand cmd(AudioCommandType::RESUME_SOUND, soundName);
    QueueCommand(cmd);
}

void AudioManager::SetSoundVolume(const std::string& soundName, float volume) {
    if (!m_Initialized.load()) return;
    
    AudioCommand cmd(AudioCommandType::SET_SOUND_VOLUME, soundName);
    cmd.value1 = std::clamp(volume, 0.0f, 1.0f);
    QueueCommand(cmd);
}

void AudioManager::SetSoundPitch(const std::string& soundName, float pitch) {
    if (!m_Initialized.load()) return;
    
    AudioCommand cmd(AudioCommandType::SET_SOUND_PITCH, soundName);
    cmd.value1 = std::max(pitch, 0.1f); // Prevent zero or negative pitch
    QueueCommand(cmd);
}

void AudioManager::SetSoundPan(const std::string& soundName, float pan) {
    if (!m_Initialized.load()) return;
    
    AudioCommand cmd(AudioCommandType::SET_SOUND_PAN, soundName);
    cmd.value1 = std::clamp(pan, 0.0f, 1.0f);
    QueueCommand(cmd);
}

bool AudioManager::LoadMusic(const std::string& musicName, const std::string& filePath) {
    if (!m_Initialized.load()) {
        SetError("AudioManager not initialized");
        return false;
    }
    
    AudioCommand cmd(AudioCommandType::LOAD_MUSIC, musicName, filePath);
    QueueCommand(cmd);
    return true;
}

void AudioManager::UnloadMusic(const std::string& musicName) {
    if (!m_Initialized.load()) return;
    
    AudioCommand cmd(AudioCommandType::UNLOAD_MUSIC, musicName);
    QueueCommand(cmd);
}

void AudioManager::PlayMusic(const std::string& musicName, bool loop) {
    if (!m_Initialized.load()) return;
    
    AudioCommand cmd(AudioCommandType::PLAY_MUSIC, musicName);
    cmd.boolValue = loop;
    QueueCommand(cmd);
}

void AudioManager::StopMusic(const std::string& musicName) {
    if (!m_Initialized.load()) return;
    
    AudioCommand cmd(AudioCommandType::STOP_MUSIC, musicName);
    QueueCommand(cmd);
}

void AudioManager::PauseMusic(const std::string& musicName) {
    if (!m_Initialized.load()) return;
    
    AudioCommand cmd(AudioCommandType::PAUSE_MUSIC, musicName);
    QueueCommand(cmd);
}

void AudioManager::ResumeMusic(const std::string& musicName) {
    if (!m_Initialized.load()) return;
    
    AudioCommand cmd(AudioCommandType::RESUME_MUSIC, musicName);
    QueueCommand(cmd);
}

void AudioManager::SetMusicVolume(const std::string& musicName, float volume) {
    if (!m_Initialized.load()) return;
    
    AudioCommand cmd(AudioCommandType::SET_MUSIC_VOLUME, musicName);
    cmd.value1 = std::clamp(volume, 0.0f, 1.0f);
    QueueCommand(cmd);
}

void AudioManager::SetMusicPitch(const std::string& musicName, float pitch) {
    if (!m_Initialized.load()) return;
    
    AudioCommand cmd(AudioCommandType::SET_MUSIC_PITCH, musicName);
    cmd.value1 = std::max(pitch, 0.1f);
    QueueCommand(cmd);
}

void AudioManager::SetMusicPan(const std::string& musicName, float pan) {
    if (!m_Initialized.load()) return;
    
    AudioCommand cmd(AudioCommandType::SET_MUSIC_PAN, musicName);
    cmd.value1 = std::clamp(pan, 0.0f, 1.0f);
    QueueCommand(cmd);
}

void AudioManager::SetMasterVolume(float volume) {
    AudioCommand cmd(AudioCommandType::SET_MASTER_VOLUME);
    cmd.value1 = std::clamp(volume, 0.0f, 1.0f);
    QueueCommand(cmd);
}

float AudioManager::GetMasterVolume() const {
    return m_MasterVolume.load();
}

void AudioManager::StopAllSounds() {
    if (!m_Initialized.load()) return;
    
    AudioCommand cmd(AudioCommandType::STOP_ALL_SOUNDS);
    QueueCommand(cmd);
}

void AudioManager::PauseAllSounds() {
    if (!m_Initialized.load()) return;
    
    AudioCommand cmd(AudioCommandType::PAUSE_ALL_SOUNDS);
    QueueCommand(cmd);
}

void AudioManager::ResumeAllSounds() {
    if (!m_Initialized.load()) return;
    
    AudioCommand cmd(AudioCommandType::RESUME_ALL_SOUNDS);
    QueueCommand(cmd);
}

void AudioManager::StopAllMusic() {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    for (auto& pair : m_LoadedMusic) {
        AudioCommand cmd(AudioCommandType::STOP_MUSIC, pair.first);
        QueueCommand(cmd);
    }
}

void AudioManager::PauseAllMusic() {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    for (auto& pair : m_LoadedMusic) {
        if (pair.second.isPlaying && !pair.second.isPaused) {
            AudioCommand cmd(AudioCommandType::PAUSE_MUSIC, pair.first);
            QueueCommand(cmd);
        }
    }
}

void AudioManager::ResumeAllMusic() {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    for (auto& pair : m_LoadedMusic) {
        if (pair.second.isPaused) {
            AudioCommand cmd(AudioCommandType::RESUME_MUSIC, pair.first);
            QueueCommand(cmd);
        }
    }
}

void AudioManager::LoadSoundBatch(const std::vector<SoundAsset>& sounds) {
    for (const auto& sound : sounds) {
        AudioCommand cmd(AudioCommandType::LOAD_SOUND, sound.name, sound.filePath);
        cmd.value1 = sound.volume;
        cmd.value2 = sound.pitch;
        cmd.value3 = sound.pan;
        QueueCommand(cmd);
    }
}

void AudioManager::LoadMusicBatch(const std::vector<MusicAsset>& music) {
    for (const auto& track : music) {
        AudioCommand cmd(AudioCommandType::LOAD_MUSIC, track.name, track.filePath);
        cmd.value1 = track.volume;
        cmd.value2 = track.pitch;
        cmd.value3 = track.pan;
        cmd.boolValue = track.loop;
        QueueCommand(cmd);
    }
}

void AudioManager::Update() {
    if (!m_Initialized.load()) {
        return;
    }
    
    // Make a local copy of the callback to avoid race conditions
    AudioEventCallback callback = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
        callback = m_EventCallback;
    }
    
    // Process queued events (thread-safe)
    std::queue<AudioEvent> eventsToProcess;
    {
        std::lock_guard<std::mutex> lock(m_EventQueueMutex);
        eventsToProcess = std::move(m_EventQueue);
        m_EventQueue = std::queue<AudioEvent>();
    }
    
    // Process events with the local copy of the callback
    while (!eventsToProcess.empty()) {
        if (callback) {
            try {
                callback(eventsToProcess.front());
            }
            catch (const std::exception& e) {
                Logger::Error<AudioManager>("Exception in audio callback: " + std::string(e.what()), this);
            }
        }
        eventsToProcess.pop();
    }
}

// Query methods
bool AudioManager::IsSoundLoaded(const std::string& soundName) const {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    return m_LoadedSounds.find(soundName) != m_LoadedSounds.end();
}

bool AudioManager::IsAudioPlaying(const std::string& soundName) const {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    auto it = m_LoadedSounds.find(soundName);
    return (it != m_LoadedSounds.end()) ? it->second.isPlaying : false;
}

bool AudioManager::IsAudioPaused(const std::string& soundName) const {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    auto it = m_LoadedSounds.find(soundName);
    return (it != m_LoadedSounds.end()) ? it->second.isPaused : false;
}

bool AudioManager::IsMusicLoaded(const std::string& musicName) const {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    return m_LoadedMusic.find(musicName) != m_LoadedMusic.end();
}

bool AudioManager::IsMusicPlaying(const std::string& musicName) const {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    auto it = m_LoadedMusic.find(musicName);
    return (it != m_LoadedMusic.end()) ? it->second.isPlaying : false;
}

bool AudioManager::IsMusicPaused(const std::string& musicName) const {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    auto it = m_LoadedMusic.find(musicName);
    return (it != m_LoadedMusic.end()) ? it->second.isPaused : false;
}

size_t AudioManager::GetLoadedSoundCount() const {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    return m_LoadedSounds.size();
}

size_t AudioManager::GetLoadedMusicCount() const {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    return m_LoadedMusic.size();
}

std::vector<std::string> AudioManager::GetLoadedSoundNames() const {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    std::vector<std::string> names;
    names.reserve(m_LoadedSounds.size());
    
    for (const auto& pair : m_LoadedSounds) {
        names.push_back(pair.first);
    }
    
    return names;
}

std::vector<std::string> AudioManager::GetLoadedMusicNames() const {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    std::vector<std::string> names;
    names.reserve(m_LoadedMusic.size());
    
    for (const auto& pair : m_LoadedMusic) {
        names.push_back(pair.first);
    }
    
    return names;
}

void AudioManager::SeekMusic(const std::string& musicName, float position) {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    auto it = m_LoadedMusic.find(musicName);
    if (it != m_LoadedMusic.end() && it->second.raudioMusic) {
        ::SeekMusicStream(*it->second.raudioMusic, position);
    }
}

float AudioManager::GetMusicTimeLength(const std::string& musicName) const {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    auto it = m_LoadedMusic.find(musicName);
    if (it != m_LoadedMusic.end() && it->second.raudioMusic) {
        return ::GetMusicTimeLength(*it->second.raudioMusic);
    }
    return 0.0f;
}

float AudioManager::GetMusicTimePlayed(const std::string& musicName) const {
    std::lock_guard<std::mutex> lock(m_AudioResourcesMutex);
    
    auto it = m_LoadedMusic.find(musicName);
    if (it != m_LoadedMusic.end() && it->second.raudioMusic) {
        return ::GetMusicTimePlayed(*it->second.raudioMusic);
    }
    return 0.0f;
}

// Global Audio namespace implementation
namespace Audio {
    static std::unique_ptr<AudioManager> g_AudioManager = nullptr;
    
    AudioManager& GetManager() {
        if (!g_AudioManager) {
            g_AudioManager = std::make_unique<AudioManager>();
        }
        return *g_AudioManager;
    }
    
    bool Initialize() {
        return GetManager().Initialize();
    }
    
    void Shutdown() {
        if (g_AudioManager) {
            Logger::Info("Shutting down audio manager");
            g_AudioManager->Shutdown();

            Logger::Info("Resetting audio manager");
            g_AudioManager.reset();
        }
    }
} 