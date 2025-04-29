# 📦 Import Libraries
import streamlit as st
import numpy as np
import matplotlib.pyplot as plt
from scipy.fft import fft, fftfreq
from sklearn.ensemble import RandomForestClassifier

# 🎯 Signal Simulation Functions

def generate_synthetic_signal(fs, duration, hr_bpm, br_bpm, noise_level=0.1):
    t = np.linspace(0, duration, int(fs*duration), endpoint=False)
    br_freq = br_bpm / 60
    breathing_signal = 0.5 * np.sin(2*np.pi*br_freq*t)
    hr_freq = hr_bpm / 60
    heart_signal = 0.2 * np.sin(2*np.pi*hr_freq*t)
    noise = noise_level * np.random.randn(len(t))
    radar_signal = breathing_signal + heart_signal + noise
    return t, radar_signal

def extract_features(signal, fs):
    N = len(signal)
    f_signal = fft(signal)
    f_signal_magnitude = np.abs(f_signal)[:N // 2]
    frequencies = fftfreq(N, 1/fs)[:N // 2]

    br_range = (frequencies >= 0.1) & (frequencies <= 0.7)
    br_peak_freq = frequencies[br_range][np.argmax(f_signal_magnitude[br_range])]
    breathing_rate = br_peak_freq * 60

    hr_range = (frequencies >= 1.0) & (frequencies <= 3.5)
    hr_peak_freq = frequencies[hr_range][np.argmax(f_signal_magnitude[hr_range])]
    heart_rate = hr_peak_freq * 60

    return breathing_rate, heart_rate

# 🧠 Train a Simple ML Model
# Settings
fs = 20  # Hz
duration = 10  # seconds
X = []
y = []

np.random.seed(42)

for _ in range(100):
    hr = np.random.uniform(60, 100)
    br = np.random.uniform(15, 30)
    _, sig = generate_synthetic_signal(fs, duration, hr, br)
    br_feat, hr_feat = extract_features(sig, fs)
    X.append([br_feat, hr_feat])
    y.append(0)

for _ in range(100):
    hr = np.random.uniform(120, 200)
    br = np.random.uniform(5, 12)
    _, sig = generate_synthetic_signal(fs, duration, hr, br)
    br_feat, hr_feat = extract_features(sig, fs)
    X.append([br_feat, hr_feat])
    y.append(1)

X = np.array(X)
y = np.array(y)

model = RandomForestClassifier(n_estimators=100, random_state=42)
model.fit(X, y)

# 🖥️ Streamlit App

st.set_page_config(page_title="Smart Pet Health Monitor", page_icon="🐶")

st.title("🐾 Smart Pet Health Monitoring System")
st.write("Monitor your pet's vital signs with UWB radar simulation!")

# Button to simulate
if st.button("🩺 Monitor New Pet"):
    # Randomly generate new dog vital signs
    new_hr = np.random.choice([np.random.uniform(70, 90), np.random.uniform(130, 180)])
    new_br = np.random.choice([np.random.uniform(18, 25), np.random.uniform(6, 10)])
    _, new_sig = generate_synthetic_signal(fs, duration, new_hr, new_br)

    new_br_feat, new_hr_feat = extract_features(new_sig, fs)
    new_features = np.array([[new_br_feat, new_hr_feat]])

    prediction = model.predict(new_features)[0]

    # Results
    st.subheader("📋 Vital Signs:")
    st.write(f"**Estimated Heart Rate:** {new_hr_feat:.2f} bpm")
    st.write(f"**Estimated Breathing Rate:** {new_br_feat:.2f} bpm")

    st.subheader("🔍 Health Status:")
    if prediction == 0:
        st.success("✅ Pet is Healthy!")
    else:
        st.error("⚠️ Pet is Unhealthy! Need Attention.")

    # Plot
    fig, ax = plt.subplots(figsize=(10,4))
    ax.plot(new_sig)
    ax.set_title("Simulated Radar Signal")
    ax.set_xlabel("Samples")
    ax.set_ylabel("Amplitude")
    ax.grid(True)
    st.pyplot(fig)

st.markdown("---")
st.caption("Created with ❤️ for smart pet care.")
