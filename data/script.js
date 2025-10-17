// User and logging setup
const users = [
    { username: "admin", password: "kali", role: "admin", photo: "admin.jpg" },
    { username: "Vans", password: "kali@254", role: "user", photo: "vans.jpg" },
    { username: "Guest", password: "Guest", role: "guest", photo: "guest.jpg" }
  ];
  
  let currentUser = null;
  let logs = [];
  
  // DOM elements for login
  const loginScreen = document.getElementById("login-screen");
  const loginBtn = document.getElementById("login-btn");
  const loginError = document.getElementById("login-error");
  const usernameInput = document.getElementById("username");
  const passwordInput = document.getElementById("password");
  
  // DOM elements for dashboard
  const dashboardScreen = document.getElementById("dashboard-screen");
  const userNameElem = document.getElementById("user-name");
  const userPhotoElem = document.getElementById("user-photo");
  const logoutBtn = document.getElementById("logout-btn");
  const toggleDarkModeBtn = document.getElementById("toggle-dark-mode");
  
  const houseLightToggle = document.getElementById("house-light-toggle");
  const houseLightStatus = document.getElementById("house-light-status");
  const ldrStatus = document.getElementById("ldr-status");
  const modelSuggestion = document.getElementById("model-suggestion");
  
  const adminLogsSection = document.getElementById("admin-logs");
  const logArea = document.getElementById("logArea");
  const exportLogsBtn = document.getElementById("export-logs");
  
  // Login Handling
  loginBtn.addEventListener("click", () => {
    const username = usernameInput.value.trim();
    const password = passwordInput.value.trim();
    const user = users.find(u => u.username === username && u.password === password);
    if (!user) {
      loginError.textContent = "Invalid credentials!";
      loginError.classList.add("shake");
      setTimeout(() => loginError.classList.remove("shake"), 500);
      return;
    }
    currentUser = user;
    loginError.textContent = "";
    loginScreen.style.display = "none";
    dashboardScreen.style.display = "block";
    document.body.classList.remove("login");
    document.body.classList.add("dashboard");
    userNameElem.textContent = user.username;
    userPhotoElem.src = user.photo;
    addLog("Logged in");
    if (user.role === "admin") {
      adminLogsSection.style.display = "block";
    }
  });
  
  // Logout Handling
  logoutBtn.addEventListener("click", () => {
    addLog("Logged out");
    currentUser = null;
    dashboardScreen.style.display = "none";
    loginScreen.style.display = "flex";
    document.body.classList.remove("dashboard");
    document.body.classList.add("login");
    usernameInput.value = "";
    passwordInput.value = "";
  });
  
  // Dark Mode Toggle
  toggleDarkModeBtn.addEventListener("click", () => {
    document.body.classList.toggle("dark");
  });
  
  // House Light Toggle Control
  let houseLightOn = false;
  houseLightToggle.addEventListener("click", () => {
    houseLightOn = !houseLightOn;
    houseLightStatus.textContent = houseLightOn ? "House Lights are On" : "House Lights are Off";
    houseLightToggle.textContent = houseLightOn ? "Turn Off House Lights" : "Turn On House Lights";
    addLog(`Manually turned ${houseLightOn ? "ON" : "OFF"} House Lights`);
    // Send command to ESP device via WebSocket (with occupancy flag update)
    socket.send(JSON.stringify({ device: "houseLight", action: "toggle", occupancy: 1 }));
  });
  
  // Logging Functions
  function addLog(action) {
    const timestamp = new Date().toLocaleTimeString();
    const logEntry = `${timestamp} - ${currentUser.username}: ${action}`;
    logs.push(logEntry);
    if (currentUser && currentUser.role === "admin") updateLogArea();
  }
  
  function updateLogArea() {
    logArea.textContent = logs.join("\n");
  }
  
  exportLogsBtn.addEventListener("click", exportLogs);
  
  function exportLogs() {
    let csvContent = "data:text/csv;charset=utf-8,Time,User,Action\n";
    logs.forEach(log => {
      let parts = log.split(" - ");
      let time = parts[0];
      let rest = parts[1].split(": ");
      let user = rest[0];
      let action = rest[1];
      csvContent += `${time},${user},${action}\n`;
    });
    const encodedUri = encodeURI(csvContent);
    const link = document.createElement("a");
    link.setAttribute("href", encodedUri);
    link.setAttribute("download", "logs.csv");
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
  }
  
  // WebSocket Connection for Sensor Data and Model Prediction Updates
  const socket = new WebSocket('ws://' + location.hostname + ':81/');
  
  socket.onopen = function() {
    console.log("WebSocket connected.");
  };
  
  socket.onmessage = function(event) {
    try {
      const data = JSON.parse(event.data);
      // Update LDR reading
      if (data.ldrValue !== undefined) {
        ldrStatus.textContent = `LDR Intensity: ${data.ldrValue}`;
      }
      // Update House Light status if provided
      if (data.houseLight !== undefined) {
        houseLightStatus.textContent = data.houseLight == 1 ? "House Lights are On" : "House Lights are Off";
        houseLightOn = data.houseLight == 1;
      }
      // Update Model Suggestion
      if (data.modelSuggestion !== undefined) {
        modelSuggestion.textContent = `Model Suggestion: ${data.modelSuggestion}`;
      }
      console.log("Data received:", data);
    } catch (e) {
      console.error("Error parsing data:", e);
    }
  };
  
  socket.onerror = function(error) {
    console.error("WebSocket Error:", error);
  };
  
  socket.onclose = function() {
    console.log("WebSocket connection closed.");
  };
  