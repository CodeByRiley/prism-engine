#pragma once

#include <string>
#include <vector>

// Basic sound data structure for configuration (renamed to avoid conflict with raudio::Sound)
struct GameSound {
    std::string name;
    std::string path;
    bool loop;
    float volume;
    float pitch;
    float pan;
    float speed;
    float pitchVariation;
    
    GameSound() : loop(false), volume(1.0f), pitch(1.0f), pan(0.5f), speed(1.0f), pitchVariation(0.0f) {}
    
    GameSound(const std::string& soundName, const std::string& soundPath, 
          bool shouldLoop = false, float vol = 1.0f, float p = 1.0f, float panning = 0.5f)
        : name(soundName), path(soundPath), loop(shouldLoop), volume(vol), pitch(p), pan(panning), speed(1.0f), pitchVariation(0.0f) {}
};

// Asset structures for batch loading
struct SoundAsset {
    std::string name;
    std::string filePath;
    float volume;
    float pitch;
    float pan;
    bool isPlaying;
    
    SoundAsset() : volume(1.0f), pitch(1.0f), pan(0.5f), isPlaying(false) {}
    
    SoundAsset(const std::string& assetName, const std::string& path, 
               float vol = 1.0f, float p = 1.0f, float panning = 0.5f, bool isPlaying = false)
        : name(assetName), filePath(path), volume(vol), pitch(p), pan(panning), isPlaying(isPlaying) {}
};

struct MusicAsset {
    std::string name;
    std::string filePath;
    float volume;
    float pitch;
    float pan;
    bool loop;
    bool isPlaying;
    
    MusicAsset() : volume(1.0f), pitch(1.0f), pan(0.5f), loop(true), isPlaying(false) {}
    
    MusicAsset(const std::string& assetName, const std::string& path, 
               bool shouldLoop = true, float vol = 1.0f, float p = 1.0f, float panning = 0.5f, bool isPlaying = false)
        : name(assetName), filePath(path), volume(vol), pitch(p), pan(panning), loop(shouldLoop), isPlaying(isPlaying) {}
};

// Audio effect parameters
struct AudioEffect {
    enum class Type {
        NONE,
        REVERB,
        ECHO,
        DISTORTION,
        FILTER_LOW_PASS,
        FILTER_HIGH_PASS,
        FILTER_BAND_PASS
    };
    
    Type type;
    float intensity;
    float decay;
    float feedback;
    float cutoffFrequency;
    float resonance;
    
    AudioEffect() : type(Type::NONE), intensity(0.0f), decay(0.0f), feedback(0.0f), 
                    cutoffFrequency(1000.0f), resonance(1.0f) {}
};

// Audio categories for organization and group control
enum class AudioCategory {
    MASTER,
    MUSIC,
    SFX,
    VOICE,
    AMBIENT,
    UI
};

// Audio settings per category
struct CategorySettings {
    AudioCategory category;
    float volume;
    bool muted;
    
    CategorySettings(AudioCategory cat = AudioCategory::MASTER, float vol = 1.0f, bool isMuted = false)
        : category(cat), volume(vol), muted(isMuted) {}
};

// Spatial audio properties
struct SpatialAudioProperties {
    bool enabled;
    float x, y, z;              // 3D position
    float directionX, directionY, directionZ; // Direction vector
    float minDistance;          // Distance at which sound starts to attenuate
    float maxDistance;          // Distance at which sound becomes inaudible
    float rolloffFactor;        // How quickly sound attenuates with distance
    float dopplerFactor;        // Doppler effect intensity
    
    SpatialAudioProperties() 
        : enabled(false), x(0.0f), y(0.0f), z(0.0f), 
          directionX(0.0f), directionY(0.0f), directionZ(1.0f),
          minDistance(1.0f), maxDistance(100.0f), rolloffFactor(1.0f), dopplerFactor(1.0f) {}
};

// Audio listener properties (for 3D audio)
struct AudioListener {
    float x, y, z;              // 3D position
    float directionX, directionY, directionZ; // Forward direction
    float upX, upY, upZ;        // Up direction
    float velocityX, velocityY, velocityZ;    // Velocity for doppler effect
    
    AudioListener() 
        : x(0.0f), y(0.0f), z(0.0f),
          directionX(0.0f), directionY(0.0f), directionZ(-1.0f),
          upX(0.0f), upY(1.0f), upZ(0.0f),
          velocityX(0.0f), velocityY(0.0f), velocityZ(0.0f) {}
};

// Preset configurations for common audio scenarios
namespace AudioPresets {
    // Music presets
    inline MusicAsset BackgroundMusic(const std::string& name, const std::string& path) {
        return MusicAsset(name, path, true, 0.7f, 1.0f, 0.5f);
    }
    
    inline MusicAsset MenuMusic(const std::string& name, const std::string& path) {
        return MusicAsset(name, path, true, 0.5f, 1.0f, 0.5f);
    }
    
    inline MusicAsset CombatMusic(const std::string& name, const std::string& path) {
        return MusicAsset(name, path, true, 0.8f, 1.0f, 0.5f);
    }
    
    // Sound effect presets
    inline SoundAsset ButtonClick(const std::string& name, const std::string& path) {
        return SoundAsset(name, path, 0.6f, 1.0f, 0.5f);
    }
    
    inline SoundAsset Explosion(const std::string& name, const std::string& path) {
        return SoundAsset(name, path, 1.0f, 1.0f, 0.5f);
    }
    
    inline SoundAsset Footstep(const std::string& name, const std::string& path) {
        return SoundAsset(name, path, 0.4f, 1.0f, 0.5f);
    }
    
    inline SoundAsset Gunshot(const std::string& name, const std::string& path) {
        return SoundAsset(name, path, 0.8f, 1.0f, 0.5f);
    }
    
    inline SoundAsset PickupItem(const std::string& name, const std::string& path) {
        return SoundAsset(name, path, 0.5f, 1.2f, 0.5f);
    }
}
