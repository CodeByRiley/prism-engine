# Threaded Audio System using raudio

This document describes the threaded audio system implementation for your game engine using the raudio library.

## Overview

The audio system is built using:
- **raudio**: A lightweight audio library based on miniaudio
- **Threading**: All audio operations run in a separate thread to prevent blocking
- **Command Queue**: Thread-safe communication between main thread and audio thread
- **Event System**: Callbacks for audio events (loaded, played, finished, etc.)

## Features

### Core Functionality
- ✅ **Threaded Audio Processing**: All audio operations run in a separate thread
- ✅ **Sound Effects**: Short audio clips (.wav, .ogg, .mp3, .flac)
- ✅ **Music Streaming**: Long audio files with streaming support
- ✅ **Volume/Pitch/Pan Control**: Per-sound and global audio controls
- ✅ **Batch Loading**: Efficient loading of multiple audio assets
- ✅ **Event Callbacks**: Real-time notifications for audio events
- ✅ **Thread-Safe API**: Safe to call from any thread

### Audio Formats Supported
- WAV (uncompressed)
- OGG Vorbis
- MP3
- FLAC
- XM (module files)
- MOD (module files)
- QOA (Quite OK Audio)

## Directory Structure

```
resources/
└── audio/
    ├── sounds/     # Short sound effects (.wav, .ogg)
    │   ├── ui_click.wav
    │   ├── footstep.wav
    │   ├── pickup.wav
    │   ├── move.wav
    │   └── hit.wav
    └── music/      # Background music (.ogg, .mp3)
        ├── background.ogg
        ├── menu.ogg
        └── action.ogg
```

## Usage Examples

### Basic Setup

```cpp
#include <engine/core/audio/AudioManager.h>

// Initialize audio system (done automatically in Game::OnInit())
Audio::Initialize();

// Set event callback
Audio::GetManager().SetEventCallback([](const AudioEvent& event) {
    Logger::Info("Audio event: " + std::to_string((int)event.type));
});
```

### Loading Sounds

```cpp
// Load individual sound
Audio::LoadSound("explosion", "resources/audio/sounds/explosion.wav");

// Load with custom settings
Audio::GetManager().LoadSound("footstep", "resources/audio/sounds/step.wav");
Audio::GetManager().SetSoundVolume("footstep", 0.3f);
Audio::GetManager().SetSoundPitch("footstep", 1.2f);

// Batch load multiple sounds
std::vector<SoundAsset> sounds = {
    AudioPresets::ButtonClick("ui_click", "resources/audio/sounds/click.wav"),
    AudioPresets::Explosion("boom", "resources/audio/sounds/explosion.wav"),
    AudioPresets::Footstep("step", "resources/audio/sounds/footstep.wav")
};
Audio::GetManager().LoadSoundBatch(sounds);
```

### Playing Sounds

```cpp
// Play sound effect
Audio::PlaySound("explosion");

// Play with custom volume
Audio::GetManager().SetSoundVolume("footstep", 0.5f);
Audio::PlaySound("footstep");

// Stop/Pause/Resume
Audio::StopSound("explosion");
Audio::PauseSound("footstep");
Audio::ResumeSound("footstep");
```

### Music Management

```cpp
// Load and play background music
Audio::LoadMusic("bg_music", "resources/audio/music/background.ogg");
Audio::PlayMusic("bg_music", true); // true = loop

// Control music
Audio::GetManager().SetMusicVolume("bg_music", 0.7f);
Audio::GetManager().PauseMusic("bg_music");
Audio::GetManager().ResumeMusic("bg_music");
Audio::StopMusic("bg_music");

// Seek to specific position (in seconds)
Audio::GetManager().SeekMusic("bg_music", 30.0f);

// Get music info
float length = Audio::GetManager().GetMusicTimeLength("bg_music");
float played = Audio::GetManager().GetMusicTimePlayed("bg_music");
```

### Global Controls

```cpp
// Master volume (0.0 to 1.0)
Audio::SetMasterVolume(0.8f);
float volume = Audio::GetManager().GetMasterVolume();

// Stop all audio
Audio::GetManager().StopAllSounds();
Audio::GetManager().StopAllMusic();

// Pause/Resume all
Audio::GetManager().PauseAllSounds();
Audio::GetManager().ResumeAllSounds();
```

### Audio Presets

The system includes convenient presets for common audio scenarios:

```cpp
// Music presets
auto bgMusic = AudioPresets::BackgroundMusic("bg", "path/to/music.ogg");
auto menuMusic = AudioPresets::MenuMusic("menu", "path/to/menu.ogg");
auto combatMusic = AudioPresets::CombatMusic("combat", "path/to/action.ogg");

// Sound effect presets
auto click = AudioPresets::ButtonClick("click", "path/to/click.wav");
auto explosion = AudioPresets::Explosion("boom", "path/to/explosion.wav");
auto footstep = AudioPresets::Footstep("step", "path/to/footstep.wav");
auto pickup = AudioPresets::PickupItem("pickup", "path/to/pickup.wav");
```

## Game Integration

### In Game::OnInit()
```cpp
void Game::SetupAudioSystem() {
    if (!Audio::Initialize()) {
        Logger::Error<Game>("Failed to initialize Audio System", this);
        return;
    }
    
    Audio::GetManager().SetEventCallback([this](const AudioEvent& event) {
        HandleAudioEvents(event);
    });
    
    LoadGameAudio();
}
```

### In Game::OnUpdate()
```cpp
void Game::OnUpdate() {
    // Update audio system (processes events)
    Audio::Update();
    
    // Your game logic...
}
```

### In Game::OnShutdown()
```cpp
void Game::OnShutdown() {
    // Shutdown audio system
    Audio::Shutdown();
}
```

## Key Bindings (Example)

The current implementation includes these example key bindings:

- **M**: Toggle background music on/off
- **N**: Play UI click sound
- **B**: Play item pickup sound
- **+/-**: Increase/decrease master volume

## Event Handling

```cpp
void Game::HandleAudioEvents(const AudioEvent& event) {
    switch (event.type) {
        case AudioEventType::SOUND_LOADED:
            Logger::Info("Sound loaded: " + event.soundName);
            break;
            
        case AudioEventType::MUSIC_STARTED:
            Logger::Info("Music started: " + event.soundName);
            break;
            
        case AudioEventType::MUSIC_FINISHED:
            // Auto-restart background music
            if (event.soundName == "background") {
                Audio::PlayMusic("background", true);
            }
            break;
            
        case AudioEventType::AUDIO_ERROR:
            Logger::Error<Game>("Audio error: " + event.message, this);
            break;
    }
}
```

## Threading Architecture

The audio system uses a dedicated thread for all audio operations:

1. **Main Thread**: Queues commands via thread-safe API
2. **Audio Thread**: Processes commands and updates music streams
3. **Event Queue**: Thread-safe communication back to main thread

This ensures:
- No audio stuttering or blocking
- Smooth music streaming
- Real-time audio processing
- Thread-safe operations

## Performance Considerations

- **Batch Loading**: Use `LoadSoundBatch()` and `LoadMusicBatch()` for multiple files
- **Music Streaming**: Long audio files are streamed, not fully loaded into memory
- **Thread Separation**: Audio processing doesn't block main thread/rendering
- **Automatic Cleanup**: Finished sounds are automatically cleaned up

## Error Handling

```cpp
// Check if operations succeeded
if (!Audio::GetManager().IsSoundLoaded("mysound")) {
    Logger::Error<Game>("Failed to load sound: " + Audio::GetManager().GetLastError(), this);
}

// Event-based error handling
Audio::GetManager().SetEventCallback([](const AudioEvent& event) {
    if (event.type == AudioEventType::AUDIO_ERROR) {
        Logger::Error<AudioManager>("Audio error: " + event.message, nullptr);
    }
});
```

## Build Requirements

The audio system is already integrated into your CMakeLists.txt:

```cmake
# raudio is already included
add_subdirectory(thirdparty/raudio)

# raudio is already linked
target_link_libraries("${CMAKE_PROJECT_NAME}" PRIVATE glm glfw 
    glad stb_image stb_truetype raudio imgui yaml-cpp enet)
```

## Next Steps

1. **Add Audio Files**: Place your .wav, .ogg, .mp3 files in `resources/audio/`
2. **Test the System**: Run the game and try the example key bindings
3. **Customize**: Modify `LoadGameAudio()` to load your specific audio assets
4. **Expand**: Add spatial audio, audio effects, or category-based volume controls

The system is fully functional and ready to use with your game engine! 