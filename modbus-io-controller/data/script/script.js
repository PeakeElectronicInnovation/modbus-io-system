// Navigation
document.querySelectorAll('nav a').forEach(link => {
    link.addEventListener('click', (e) => {
        e.preventDefault();
        const page = e.target.dataset.page;
        document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
        document.querySelector(`#${page}`).classList.add('active');
        document.querySelectorAll('nav a').forEach(a => a.classList.remove('active'));
        e.target.classList.add('active');
    });
});

async function updateNetworkInfo() {
    const settingsTab = document.getElementById('settings');
    if (!settingsTab || !settingsTab.classList.contains('active')) return;
    try {
        const response = await fetch('/api/network');
        const settings = await response.json();
        const currentIPElement = document.getElementById('currentIP');
        const macAddressElement = document.getElementById('macAddress');
        
        if (currentIPElement) {
            currentIPElement.textContent = settings.ip || 'Not Connected';
        }
        if (macAddressElement) {
            macAddressElement.textContent = settings.mac || '';
        }
    } catch (error) {
        console.error('Error updating network info:', error);
    }
}

async function saveTimeSettings() {
    const date = document.getElementById('currentDate').value;
    const fullTime = document.getElementById('currentTime').value;
    // Extract only hours and minutes from the time value
    const timeParts = fullTime.split(':');
    const time = `${timeParts[0]}:${timeParts[1]}`; // HH:MM format
    const timezone = document.getElementById('timezone').value;
    const ntpEnabled = document.getElementById('enableNTP').checked;
    const dstEnabled = document.getElementById('enableDST').checked;

    // Show loading toast
    const loadingToast = showToast('info', 'Saving...', 'Updating date and time settings', 10000);

    try {
        const response = await fetch('/api/time', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({
                date,
                time,
                timezone,
                ntpEnabled,
                dstEnabled
            }),
        });

        // Remove loading toast
        if (loadingToast.parentNode) {
            loadingToast.parentNode.removeChild(loadingToast);
        }

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const result = await response.json();
        console.log('Time settings updated:', result);
        
        // Show success toast
        showToast('success', 'Success', 'Date and time settings saved successfully');
        
        // Update the time display immediately
        updateLiveClock();
    } catch (error) {
        console.error('Error saving time settings:', error);
        
        // Show error toast
        showToast('error', 'Error', 'Failed to save date and time settings');
    }
}

// Update live clock display
async function updateLiveClock() {
    // Check if we are in the settings tab or status tab
    const settingsTab = document.getElementById('settings');
    const statusTab = document.getElementById('system');
    if ((!settingsTab || !settingsTab.classList.contains('active')) && (!statusTab || !statusTab.classList.contains('active'))) return;
    
    const clockElement = document.getElementById('liveClock');
    if (clockElement) {
        try {
            const response = await fetch('/api/time');
            const data = await response.json();
            console.log('Time data received:', data);

            if (data.date && data.time && data.timezone) {
                const isoDateTime = `${data.date}T${data.time}${data.timezone}`;

               const now = new Date(isoDateTime);

               const formattedTime = new Intl.DateTimeFormat('en-NZ', {
                    year: 'numeric',
                    month: 'long',
                    day: 'numeric',
                    hour: '2-digit',
                    minute: '2-digit',
                    second: '2-digit',
                    timeZone: data.timezone
                }).format(now) + ' ' + data.timezone + (data.dst ? ' (DST)' : '');
           
                clockElement.textContent = formattedTime;
            }

            // Update NTP status if NTP is enabled
            const ntpStatusContainer = document.getElementById('ntpStatusContainer');
            const ntpStatusElement = document.getElementById('ntpStatus');
            const lastNtpUpdateElement = document.getElementById('lastNtpUpdate');
            
            if (data.ntpEnabled) {
                ntpStatusContainer.style.display = 'block';
                
                if (ntpStatusElement && data.hasOwnProperty('ntpStatus')) {
                    // Clear previous classes
                    ntpStatusElement.className = 'ntp-status';
                    
                    // Add status text and class
                    let statusText = '';
                    switch (data.ntpStatus) {
                        case 0: // NTP_STATUS_CURRENT
                            statusText = 'Current';
                            ntpStatusElement.classList.add('ntp-status-current');
                            break;
                        case 1: // NTP_STATUS_STALE
                            statusText = 'Stale';
                            ntpStatusElement.classList.add('ntp-status-stale');
                            break;
                        case 2: // NTP_STATUS_FAILED
                        default:
                            statusText = 'Failed';
                            ntpStatusElement.classList.add('ntp-status-failed');
                            break;
                    }
                    ntpStatusElement.textContent = statusText;
                }
                
                if (lastNtpUpdateElement && data.hasOwnProperty('lastNtpUpdate')) {
                    lastNtpUpdateElement.textContent = data.lastNtpUpdate;
                }
            } else {
                ntpStatusContainer.style.display = 'none';
            }
        } catch (error) {
            console.error('Error updating clock:', error);
        }
    }
}

// Update date/time input fields based on NTP state
function updateInputStates() {
    const ntpEnabled = document.getElementById('enableNTP').checked;
    const dateInput = document.getElementById('currentDate');
    const timeInput = document.getElementById('currentTime');
    
    if (dateInput && timeInput) {
        dateInput.disabled = ntpEnabled;
        timeInput.disabled = ntpEnabled;
    }
}

// Function to load initial settings
async function loadInitialSettings() {
    try {
        const response = await fetch('/api/time');
        const data = await response.json();
        
        if (data) {
            document.getElementById('enableNTP').checked = data.ntpEnabled;
            document.getElementById('enableDST').checked = data.dst;
            document.getElementById('timezone').value = data.timezone;
            updateInputStates();
        }
    } catch (error) {
        console.error('Error loading initial settings:', error);
    }
}

// Function to load initial network settings
async function loadNetworkSettings() {    
    try {
        const response = await fetch('/api/network');
        const data = await response.json();
        console.log('Network settings received:', data);
        
        if (data) {
            // Set IP configuration mode
            const ipConfig = document.getElementById('ipConfig');
            if (ipConfig) {
                ipConfig.value = data.mode;
                // Trigger the change event to show/hide static fields
                ipConfig.dispatchEvent(new Event('change'));
            }

            // Set static IP fields
            document.getElementById('ipAddress').value = data.ip || '';
            document.getElementById('subnetMask').value = data.subnet || '';
            document.getElementById('gateway').value = data.gateway || '';
            document.getElementById('dns').value = data.dns || '';

            // Set NTP server and hostname
            document.getElementById('hostName').value = data.hostname || '';
            document.getElementById('ntpServer').value = data.ntp || '';
            
            // Set Modbus TCP port
            document.getElementById('modbusTcpPort').value = data.modbusTcpPort || 502;
        }
    } catch (error) {
        console.error('Error loading network settings:', error);
    }
}


// Load version information
async function loadVersionInfo() {
    try {
        const response = await fetch('/api/system/version');
        const versionData = await response.json();
        const versionElement = document.getElementById('versionInfo');
        if (versionElement && versionData.version_string) {
            versionElement.textContent = versionData.version_string;
        }
    } catch (error) {
        console.error('Error loading version info:', error);
        const versionElement = document.getElementById('versionInfo');
        if (versionElement) {
            versionElement.textContent = 'Modbus TCP IO System';
        }
    }
}

// Event listeners
document.addEventListener('DOMContentLoaded', () => {
    console.log('DOM loaded - initialising board configuration system');
    
    // Load version information
    loadVersionInfo();
    
    // Load board configurations on page load
    loadBoardConfigurations();    
    loadInitialSettings();  // Load initial NTP and timezone settings
    loadNetworkSettings();  // Load initial network settings
    updateLiveClock();
    updateNetworkInfo();    // Update network status info
    
    // Initialise system status if system tab is active initially
    if (document.querySelector('#system').classList.contains('active')) {
        updateSystemStatus();
    }
    
    // Set up event listeners
    const ntpCheckbox = document.getElementById('enableNTP');
    if (ntpCheckbox) {
        ntpCheckbox.addEventListener('change', updateInputStates);
    }
    
    // Set up save button handler
    const saveButton = document.getElementById('saveTimeBtn');
    if (saveButton) {
        saveButton.addEventListener('click', saveTimeSettings);
    }

    // Set up IP configuration mode handler
    const ipConfig = document.getElementById('ipConfig');
    if (ipConfig) {
        const updateStaticFields = () => {
            const staticFields = document.getElementById('staticSettings');
            if (staticFields) {
                staticFields.style.display = ipConfig.value === 'static' ? 'block' : 'none';
            }
        };
        ipConfig.addEventListener('change', updateStaticFields);
        // Set initial state
        updateStaticFields();
    }
});

// Update intervals
setInterval(updateLiveClock, 1000);         // Update clock every second
setInterval(updateNetworkInfo, 5000);       // Update network info every 5 seconds
setInterval(updateBoardsListStatus, 2000);  // Update board connection status every 2 seconds

// Update system status information
async function updateSystemStatus() {
    try {
        const response = await fetch('/api/system/status');
        const data = await response.json();
        
        // Update power supplies
        if (data.power) {
            // Main voltage
            document.getElementById('mainVoltage').textContent = data.power.mainVoltage.toFixed(1) + 'V';
            const mainStatus = document.getElementById('mainVoltageStatus');
            mainStatus.textContent = data.power.mainVoltageOK ? 'OK' : 'OUT OF RANGE';
            mainStatus.className = 'status ' + (data.power.mainVoltageOK ? 'ok' : 'error');
        }
        
        // Update RTC status
        if (data.rtc) {
            const rtcStatus = document.getElementById('rtcStatus');
            rtcStatus.textContent = data.rtc.ok ? 'OK' : 'ERROR';
            rtcStatus.className = 'status ' + (data.rtc.ok ? 'ok' : 'error');
            
            document.getElementById('rtcTime').textContent = data.rtc.time;
        }
              
        // Update Modbus status with enhanced logic
        const modbusStatus = document.getElementById('modbusStatus');
        if (data.modbus && data.modbus.busy) {
            modbusStatus.textContent = 'BUSY';
            modbusStatus.className = 'status warning';
        } else if (data.modbus && data.modbus.hasOfflineBoards) {
            modbusStatus.textContent = 'BOARDS OFFLINE';
            modbusStatus.className = 'status warning';
        } else if (data.modbus && data.modbus.connected) {
            modbusStatus.textContent = 'CONNECTED';
            modbusStatus.className = 'status connected';
        } else {
            modbusStatus.textContent = 'NOT-CONNECTED';
            modbusStatus.className = 'status not-connected';
        }
        
        // Update Modbus TCP status
        if (data.modbusTcp) {
            console.log('Full modbusTcp data:', data.modbusTcp);
            const modbusTcpStatus = document.getElementById('modbusTcpStatus');
            const modbusTcpPortElement = document.getElementById('modbusTcpPortStatus');
            const modbusTcpClients = document.getElementById('modbusTcpClients');
            const modbusTcpClientDetails = document.getElementById('modbusTcpClientDetails');
            const modbusTcpClientList = document.getElementById('modbusTcpClientList');
            
            // Update status in Communications panel
            if (modbusTcpStatus) {
                if (data.modbusTcp.enabled) {
                    modbusTcpStatus.textContent = 'ENABLED';
                    modbusTcpStatus.className = 'status ok';
                } else {
                    modbusTcpStatus.textContent = 'DISABLED';
                    modbusTcpStatus.className = 'status warning';
                }
            }
            
            // Update port in Communications panel
            if (modbusTcpPortElement) {
                modbusTcpPortElement.textContent = data.modbusTcp.port || 502;
            }
            
            // Update client count in Modbus TCP Clients panel
            if (modbusTcpClients) {
                modbusTcpClients.textContent = data.modbusTcp.connectedClients || 0;
            }
            
            // Update client details in Modbus TCP Clients panel
            if (modbusTcpClientDetails && modbusTcpClientList) {
                if (data.modbusTcp.clients && data.modbusTcp.clients.length > 0) {
                    modbusTcpClientDetails.style.display = 'block';
                    modbusTcpClientList.innerHTML = '';
                    
                    data.modbusTcp.clients.forEach(clientInfo => {
                        const clientDiv = document.createElement('div');
                        clientDiv.className = 'client-item';
                        clientDiv.textContent = clientInfo;
                        modbusTcpClientList.appendChild(clientDiv);
                    });
                } else {
                    modbusTcpClientDetails.style.display = 'none';
                }
            }
        }
        
        // Update SD card status
        if (data.sd) {
            const sdStatus = document.getElementById('sdStatus');
            if (!data.sd.inserted) {
                sdStatus.textContent = 'NOT INSERTED';
                sdStatus.className = 'status warning';
                hideSDDetails();
            } else if (!data.sd.ready) {
                sdStatus.textContent = 'ERROR';
                sdStatus.className = 'status error';
                hideSDDetails();
            } else {
                sdStatus.textContent = 'OK';
                sdStatus.className = 'status ok';
                
                // Show SD card details
                document.getElementById('sdCapacityContainer').style.display = 'flex';
                document.getElementById('sdFreeSpaceContainer').style.display = 'flex';
                
                // Update SD card details
                document.getElementById('sdCapacity').textContent = data.sd.capacityGB.toFixed(2) + ' GB';
                document.getElementById('sdFreeSpace').textContent = data.sd.freeSpaceGB.toFixed(2) + ' GB';
            }
        }
    } catch (error) {
        console.error('Error updating system status:', error);
    }
}

// Helper function to hide SD card details
function hideSDDetails() {
    document.getElementById('sdCapacityContainer').style.display = 'none';
    document.getElementById('sdFreeSpaceContainer').style.display = 'none';
}

// Update system status when the system tab is active
function updateStatusIfSystemTabActive() {
    if (document.querySelector('#system').classList.contains('active')) {
        updateSystemStatus();
    }
}

// Add system status update to the periodic updates
setInterval(updateStatusIfSystemTabActive, 1000);

// When switching to the system tab, update the status immediately
document.querySelectorAll('nav a[data-page="system"]').forEach(link => {
    link.addEventListener('click', () => {
        updateSystemStatus();
    });
});

// Toast notification functions
function showToast(type, title, message, duration = 3000) {
    const toastContainer = document.getElementById('toastContainer');
    
    const toast = document.createElement('div');
    toast.className = `toast toast-${type}`;
    
    let iconClass = '';
    switch (type) {
        case 'success': iconClass = '✓'; break;
        case 'error': iconClass = '✗'; break;
        case 'info': iconClass = 'ℹ'; break;
        default: iconClass = 'ℹ';
    }
    
    toast.innerHTML = `
        <div class="toast-icon">${iconClass}</div>
        <div class="toast-content">
            <div class="toast-title">${title}</div>
            <div class="toast-message">${message}</div>
        </div>
    `;
    
    toastContainer.appendChild(toast);
    
    setTimeout(() => {
        toast.classList.add('toast-exit');
        setTimeout(() => {
            if (toastContainer.contains(toast)) {
                toastContainer.removeChild(toast);
            }
        }, 300); // Exit animation delay
    }, duration);
    
    return toast;
}

// Network settings handling
document.getElementById('networkForm').addEventListener('submit', async function(e) {
    e.preventDefault();
    const statusDiv = document.getElementById('networkStatus');
    statusDiv.textContent = '';
    statusDiv.className = '';
    
    // Validate IP addresses if static IP is selected
    if (document.getElementById('ipConfig').value === 'static') {
        const inputs = ['ipAddress', 'subnetMask', 'gateway', 'dns'];
        for (const id of inputs) {
            const input = document.getElementById(id);
            if (!input.checkValidity()) {
                showToast('error', 'Validation Error', `Invalid ${id.replace(/([A-Z])/g, ' $1').toLowerCase()}`);
                return;
            }
        }
    }

    const networkConfig = {
        mode: document.getElementById('ipConfig').value,
        ip: document.getElementById('ipAddress').value,
        subnet: document.getElementById('subnetMask').value,
        gateway: document.getElementById('gateway').value,
        dns: document.getElementById('dns').value,
        hostname: document.getElementById('hostName').value,
        ntp: document.getElementById('ntpServer').value,
        dst: document.getElementById('enableDST').checked,
        modbusTcpPort: parseInt(document.getElementById('modbusTcpPort').value)
    };

    // Show loading toast
    const loadingToast = showToast('info', 'Saving...', 'Updating network settings', 10000);

    try {
        const response = await fetch('/api/network', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(networkConfig)
        });

        // Remove loading toast
        if (loadingToast.parentNode) {
            loadingToast.parentNode.removeChild(loadingToast);
        }

        const result = await response.json();
        
        if (response.ok) {
            const restartToast = showToast('success', 'Success', 'Network settings saved. The device will restart to apply changes...', 5000);
            
            // Wait a moment before reloading to show the message
            setTimeout(() => {
                window.location.reload();
            }, 5000);
        } else {
            showToast('error', 'Error', result.error || 'Failed to save network settings');
        }
    } catch (error) {
        // Remove loading toast if it still exists
        if (loadingToast.parentNode) {
            loadingToast.parentNode.removeChild(loadingToast);
        }
        
        showToast('error', 'Network Error', error.message || 'Failed to connect to the server');
    }
});

// Show/hide static IP settings
document.getElementById('ipConfig').addEventListener('change', function() {
    const staticSettings = document.getElementById('staticSettings');
    if (this.value === 'static') {
        staticSettings.classList.remove('hidden');
    } else {
        staticSettings.classList.add('hidden');
    }
});

// System reboot functionality
document.addEventListener('DOMContentLoaded', () => {
    const rebootButton = document.getElementById('rebootButton');
    const rebootModal = document.getElementById('rebootModal');
    const cancelReboot = document.getElementById('cancelReboot');
    const confirmReboot = document.getElementById('confirmReboot');

    if (rebootButton) {
        rebootButton.addEventListener('click', () => {
            rebootModal.classList.add('active');
        });
    }

    if (cancelReboot) {
        cancelReboot.addEventListener('click', () => {
            rebootModal.classList.remove('active');
        });
    }

    if (confirmReboot) {
        confirmReboot.addEventListener('click', async () => {
            try {
                // Show a loading state
                confirmReboot.textContent = 'Rebooting...';
                confirmReboot.disabled = true;
                
                // Call the reboot API
                const response = await fetch('/api/system/reboot', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json'
                    }
                });
                
                if (response.ok) {
                    // Hide the modal
                    rebootModal.classList.remove('active');
                    
                    // Show a toast notification
                    showToast('info', 'System Rebooting', 'The system is rebooting. This page will be unavailable for a few moments.');
                    
                    // Add a countdown to reconnect
                    let countdown = 12;
                    const reconnectMessage = document.createElement('div');
                    reconnectMessage.className = 'reconnect-message';
                    reconnectMessage.innerHTML = `<p>Reconnecting in <span id="countdown">${countdown}</span> seconds...</p>`;
                    document.body.appendChild(reconnectMessage);
                    
                    const countdownInterval = setInterval(() => {
                        countdown--;
                        document.getElementById('countdown').textContent = countdown;
                        
                        if (countdown <= 0) {
                            clearInterval(countdownInterval);
                            window.location.reload();
                        }
                    }, 1000);
                } else {
                    throw new Error(`HTTP error! status: ${response.status}`);
                }
            } catch (error) {
                console.error('Error rebooting system:', error);
                showToast('error', 'Reboot Failed', 'Failed to reboot the system. Please try again.');
                
                // Reset the button state
                confirmReboot.textContent = 'Yes, Reboot';
                confirmReboot.disabled = false;
                
                // Hide the modal
                rebootModal.classList.remove('active');
            }
        });
    }
});

// File Manager functionality
let fileManagerActive = false;
let sdStatusInterval = null;
let currentPath = '/';

// Function to load directory contents with retry
function loadDirectory(path, retryCount = 0) {
    if (!fileManagerActive) return;
    
    const fileListContainer = document.getElementById('file-list-items');
    if (!fileListContainer) return;
    
    const maxRetries = 3;
    fileListContainer.innerHTML = '<div class="loading">Loading files...</div>';
    updatePathNavigator(path);
    
    // Add a short delay before first API call after initialisation
    const initialDelay = (retryCount === 0) ? 500 : 0;
    
    setTimeout(() => {
        fetch(`/api/sd/list?path=${encodeURIComponent(path)}`)
            .then(response => {
                if (!response.ok) {
                    // If we get a non-OK response but the SD is inserted according to the system status
                    // it could be a timing issue with the SD card initialisation
                    if (response.status === 503 && retryCount < maxRetries) {
                        // Wait longer with each retry
                        const delay = 1000 * (retryCount + 1);
                        console.log(`SD card not ready, retrying in ${delay}ms (attempt ${retryCount + 1}/${maxRetries})`);
                        fileListContainer.innerHTML = `<div class="loading">SD card initialising, please wait... (${retryCount + 1}/${maxRetries})</div>`;
                        
                        setTimeout(() => {
                            loadDirectory(path, retryCount + 1);
                        }, delay);
                        return null; // Skip further processing for this attempt
                    }
                    
                    throw new Error(response.status === 503 ? 
                        'SD card not ready. It may be initialising or experiencing an issue.' :
                        'Failed to list directory');
                }
                return response.json();
            })
            .then(data => {
                if (data) {
                    displayDirectoryContents(data);
                    currentPath = path; // Update the current path after successful load
                }
            })
            .catch(error => {
                console.error('Error loading directory:', error);
                fileListContainer.innerHTML = `
                    <div class="error-message">
                        ${error.message || 'Failed to load directory contents'}
                        ${retryCount >= maxRetries ? 
                            '<div class="retry-action"><button id="retryButton" class="download-btn">Retry</button></div>' : ''}
                    </div>
                `;
                
                // Add retry button functionality
                const retryButton = document.getElementById('retryButton');
                if (retryButton) {
                    retryButton.addEventListener('click', () => {
                        loadDirectory(path, 0); // Reset retry count on manual retry
                    });
                }
            });
    }, initialDelay);
}

// Fix to ensure SD card operation succeeds after system boot
function checkAndLoadDirectory() {
    if (!fileManagerActive) return;
    
    const fileListContainer = document.getElementById('file-list-items');
    const pathNavigator = document.getElementById('path-navigator');
    const sdStatusElement = document.getElementById('sd-status');
    
    if (!fileListContainer || !sdStatusElement) return;
    
    fetch('/api/system/status')
        .then(response => response.json())
        .then(data => {
            if (data.sd && data.sd.inserted && data.sd.ready) {
                // If SD card is ready according to status API, try loading the directory
                loadDirectory(currentPath);
            } else if (data.sd && data.sd.inserted) {
                // SD card is inserted but not fully ready - wait and retry
                sdStatusElement.innerHTML = '<div class="status-info">SD Card initialising, please wait...</div>';
                setTimeout(checkAndLoadDirectory, 1500);
            } else {
                // SD card not inserted
                sdStatusElement.innerHTML = '<div class="status-error">SD Card not inserted</div>';
                fileListContainer.innerHTML = '<div class="error-message">SD Card is not inserted. Please insert an SD card to view files.</div>';
                if (pathNavigator) pathNavigator.innerHTML = '';
            }
        })
        .catch(error => {
            console.error('Error checking SD status:', error);
            // Retry after a delay
            setTimeout(checkAndLoadDirectory, 2000);
        });
}

// Function to check SD card status
function checkSDCardStatus() {
    if (!fileManagerActive) return;
    
    const fileListContainer = document.getElementById('file-list-items');
    const pathNavigator = document.getElementById('path-navigator');
    const sdStatusElement = document.getElementById('sd-status');
    
    if (!fileListContainer || !sdStatusElement) return;
    
    fetch('/api/system/status')
        .then(response => response.json())
        .then(data => {
            if (!data.sd.inserted) {
                // SD card is physically not inserted
                sdStatusElement.innerHTML = '<div class="status-error">SD Card not inserted</div>';
                fileListContainer.innerHTML = '<div class="error-message">SD Card is not inserted. Please insert an SD card to view files.</div>';
                // Disable the file manager functionality when SD card is not available
                if (pathNavigator) pathNavigator.innerHTML = '';
            } else {
                // SD card is physically present - show basic info regardless of ready status
                if (data.sd.ready && data.sd.capacityGB) {
                    sdStatusElement.innerHTML = `<div class="status-good">SD Card Ready - ${data.sd.freeSpaceGB.toFixed(2)} GB free of ${data.sd.capacityGB.toFixed(2)} GB</div>`;
                } else {
                    sdStatusElement.innerHTML = '<div class="status-info">SD Card inserted</div>';
                }
                
                // Only reload directory contents if the file manager shows the "not inserted" error message
                const errorMsg = fileListContainer.querySelector('.error-message');
                if (errorMsg && errorMsg.textContent.includes('not inserted')) {
                    checkAndLoadDirectory();
                }
            }
        })
        .catch(error => {
            console.error('Error checking SD status:', error);
        });
}

// Initialise the file manager
function initFileManager() {
    if (!document.getElementById('filemanager')) return;
    
    console.log('Initialising file manager');
    fileManagerActive = true;
    
    // Initialise file manager
    checkSDCardStatus();
    
    // Start periodic SD card status checks
    if (sdStatusInterval) {
        clearInterval(sdStatusInterval);
    }
    sdStatusInterval = setInterval(checkSDCardStatus, 3000); // Check every 3 seconds
    
    // Initialise with our improved function
    checkAndLoadDirectory();
}

// Navigate to a directory
function navigateTo(path) {
    if (fileManagerActive) {
        loadDirectory(path);
    }
}

// Initialise File Manager when switching to the tab
document.addEventListener('DOMContentLoaded', () => {
    // Add tab switching behavior for File Manager tab
    const fileManagerTab = document.querySelector('a[data-page="filemanager"]');
    if (fileManagerTab) {
        fileManagerTab.addEventListener('click', () => {
            fileManagerActive = true;
            initFileManager();
        });
    }
    
    // Handle switching away from file manager
    document.querySelectorAll('nav a:not([data-page="filemanager"])').forEach(tab => {
        tab.addEventListener('click', () => {
            fileManagerActive = false;
            // Clear the interval when leaving the file manager tab
            if (sdStatusInterval) {
                clearInterval(sdStatusInterval);
                sdStatusInterval = null;
            }
        });
    });
    
    // Initialise if file manager is the active tab on page load
    if (document.querySelector('#filemanager.active')) {
        fileManagerActive = true;
        initFileManager();
    }
});

// Function to format file size in a human-readable format
function formatFileSize(bytes) {
    if (bytes === 0) return '0 B';
    
    const units = ['B', 'KB', 'MB', 'GB', 'TB'];
    const i = Math.floor(Math.log(bytes) / Math.log(1024));
    return parseFloat((bytes / Math.pow(1024, i)).toFixed(2)) + ' ' + units[i];
}

// Function to update the path navigator
function updatePathNavigator(path) {
    const pathNavigator = document.getElementById('path-navigator');
    if (!pathNavigator) return;
    
    pathNavigator.innerHTML = '';
    
    const parts = path.split('/').filter(part => part !== '');
    
    // Add root
    const rootElement = document.createElement('span');
    rootElement.className = 'path-part';
    rootElement.textContent = 'Root';
    rootElement.onclick = () => navigateTo('/');
    pathNavigator.appendChild(rootElement);
    
    let currentPathBuilder = '';
    parts.forEach((part, index) => {
        // Add separator
        const separator = document.createElement('span');
        separator.className = 'path-separator';
        separator.textContent = ' / ';
        pathNavigator.appendChild(separator);
        
        // Add path part
        currentPathBuilder += '/' + part;
        const pathPart = document.createElement('span');
        pathPart.className = 'path-part';
        pathPart.textContent = part;
        
        // Only make it clickable if it's not the last part
        if (index < parts.length - 1) {
            const pathCopy = currentPathBuilder; // Create a closure copy
            pathPart.onclick = () => navigateTo(pathCopy);
        }
        pathNavigator.appendChild(pathPart);
    });
}

// Function to display directory contents
function displayDirectoryContents(data) {
    const fileListContainer = document.getElementById('file-list-items');
    if (!fileListContainer) return;
    
    fileListContainer.innerHTML = '';
    
    // Constant for maximum file size (should match server MAX_DOWNLOAD_SIZE)
    const MAX_DOWNLOAD_SIZE = 5242880; // 5MB in bytes
    
    // Check if there are no files or directories
    if (data.directories.length === 0 && data.files.length === 0) {
        fileListContainer.innerHTML = '<div class="empty-message">This directory is empty</div>';
        return;
    }
    
    // Display directories first
    data.directories.forEach(dir => {
        const dirElement = document.createElement('div');
        dirElement.className = 'directory-item';
        
        // Get readable directory name
        const dirName = dir.name;
        
        dirElement.innerHTML = `
            <div class="file-name">${dirName}</div>
            <div class="file-size">Directory</div>
            <div class="file-modified">-</div>
            <div class="file-actions"></div>
        `;
        dirElement.onclick = () => navigateTo(dir.path);
        fileListContainer.appendChild(dirElement);
    });
    
    // Then display files
    data.files.forEach(file => {
        const fileElement = document.createElement('div');
        fileElement.className = 'file-item';
        
        // Check if file is too large for download
        const isTooLarge = file.size > MAX_DOWNLOAD_SIZE;
        const downloadBtnClass = isTooLarge ? 'download-btn disabled' : 'download-btn';
        const downloadBtnTitle = isTooLarge 
            ? `File is too large to download (${formatFileSize(file.size)}). Maximum size is ${formatFileSize(MAX_DOWNLOAD_SIZE)}.`
            : 'Download this file';
        
        fileElement.innerHTML = `
            <div class="file-name" data-path="${file.path}">${file.name}</div>
            <div class="file-size">${formatFileSize(file.size)}</div>
            <div class="file-modified">${file.modified || '-'}</div>
            <div class="file-actions">
                <button class="${downloadBtnClass}" data-path="${file.path}" title="${downloadBtnTitle}">Download</button>
            </div>
        `;
        fileListContainer.appendChild(fileElement);
        
        // Add click handler to filename for viewing
        const fileName = fileElement.querySelector('.file-name');
        fileName.addEventListener('click', (event) => {
            event.stopPropagation(); // Prevent bubbling
            const filePath = event.target.getAttribute('data-path');
            viewFile(filePath);
        });
        
        // Add download button event listener (only if not too large)
        const downloadBtn = fileElement.querySelector('.download-btn');
        if (!isTooLarge) {
            downloadBtn.addEventListener('click', (event) => {
                event.stopPropagation(); // Prevent directory click event
                const filePath = event.target.getAttribute('data-path');
                downloadFile(filePath);
            });
        }
    });
}

// Function to download a file
function downloadFile(path) {
    // Extract the filename from the path
    const filename = path.split('/').pop();
    const downloadUrl = `/api/sd/download?path=${encodeURIComponent(path)}`;
    
    // Create a link and click it to start download
    const a = document.createElement('a');
    a.href = downloadUrl;
    // Explicitly set the download attribute with the filename
    a.download = filename; 
    a.setAttribute('download', filename); // For older browsers
    // Don't use _blank as it tends to open in browser
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
}

// Function to view a file in the browser
function viewFile(path) {
    // Construct the URL for viewing the file
    const viewUrl = `/api/sd/view?path=${encodeURIComponent(path)}`;
    
    // Open file in a new tab
    window.open(viewUrl, '_blank');
}

// Board Configuration
let boardConfigurations = [];
let editingBoardIndex = null;
let connectStatusGlobal = [];
let initStatusGlobal = [];

// Function to convert board type string to enum value
function getBoardTypeValue(boardType) {
    switch (boardType) {
        case 'CONTROLLER_IO':
            return 0;
        case 'ANALOGUE_DIGITAL_IO':
            return 1;
        case 'THERMOCOUPLE_IO':
            return 2;
        case 'RTD_IO':
            return 3;
        case 'ENERGY_METER':
            return 4;
        default:
            return 0; // Default to master controller
    }
}

// Load board configurations from backend (optimized for binary-backed API)
async function loadBoardConfigurations() {
    console.log("Loading board configurations from binary-backed API...");
    
    const maxRetries = 3;
    let retryCount = 0;
    let lastError = null;
    
    while (retryCount < maxRetries) {
        try {
            // Add cache-busting parameter and optimize headers for binary backend
            const timestamp = new Date().getTime();
            const endpoint = `/api/boards/all?t=${timestamp}`;
            
            console.log(`Attempt ${retryCount + 1}/${maxRetries} - Fetching from optimized endpoint: ${endpoint}`);
            
            const response = await fetch(endpoint, {
                method: 'GET',
                headers: {
                    'Accept': 'application/json',
                    'Cache-Control': 'no-cache',
                    'Pragma': 'no-cache'
                }
            });
            
            if (response.ok) {
                console.log(`✓ Successful response from binary-backed API`);
                const responseText = await response.text();
                
                // Enhanced JSON parsing with better error reporting
                let data;
                try {
                    data = JSON.parse(responseText);
                } catch (parseError) {
                    console.error("JSON parse error:", parseError);
                    console.error("Raw response:", responseText.substring(0, 500));
                    throw new Error(`Invalid JSON response: ${parseError.message}`);
                }
                
                // Validate response structure
                if (!data || typeof data !== 'object') {
                    throw new Error('Invalid response format: expected object');
                }
                
                if (data.boards && Array.isArray(data.boards)) {
                    boardConfigurations = data.boards;
                    console.log(`✓ Loaded ${boardConfigurations.length} board configurations from binary storage`);
                    
                    // Validate each board configuration
                    boardConfigurations.forEach((board, index) => {
                        if (!board.hasOwnProperty('id') || !board.hasOwnProperty('name') || !board.hasOwnProperty('type')) {
                            console.warn(`Board ${index} missing required properties:`, board);
                        }
                        if (board.type === 2 && (!board.channels || !Array.isArray(board.channels))) {
                            console.warn(`Thermocouple board ${index} missing channels array:`, board);
                        }
                    });
                    
                    if (boardConfigurations.length > 0) {
                        console.log("Sample board data:", boardConfigurations[0]);
                    }
                } else {
                    boardConfigurations = [];
                    console.log("No boards found in response - initializing empty array");
                }
                
                renderBoardsList();
                return; // Success - exit retry loop
                
            } else {
                const errorText = await response.text();
                throw new Error(`HTTP ${response.status}: ${errorText}`);
            }
            
        } catch (error) {
            lastError = error;
            console.error(`Attempt ${retryCount + 1}/${maxRetries} failed:`, error.message);
            
            retryCount++;
            if (retryCount < maxRetries) {
                const delay = Math.min(Math.pow(2, retryCount) * 500, 5000); // Cap at 5s
                console.log(`Retrying in ${delay}ms...`);
                await new Promise(resolve => setTimeout(resolve, delay));
            }
        }
    }
    
    // All retries failed
    console.error('✗ Failed to load board configurations after all retries:', lastError?.message);
    showToast('error', 'Configuration Error', 'Unable to load board configurations. Please check device connection.');
    boardConfigurations = [];
    renderBoardsList();
}

// Enhanced board request handler for binary-backed API
async function sendBoardRequest(url, method, data, successMessage) {
    const maxRetries = 3;
    let retryCount = 0;
    let lastError = null;
    
    console.log(`Sending ${method} request to binary-backed API:`, url);
    console.log('Request data:', JSON.stringify(data, null, 2));
    
    while (retryCount < maxRetries) {
        try {
            console.log(`Attempt ${retryCount + 1}/${maxRetries} - ${method} ${url}`);
            
            const response = await fetch(url, {
                method: method,
                headers: {
                    'Content-Type': 'application/json',
                    'Accept': 'application/json',
                    'Cache-Control': 'no-cache',
                    'Pragma': 'no-cache'
                },
                body: JSON.stringify(data)
            });
            
            if (!response.ok) {
                const errorText = await response.text();
                console.error(`Server error (${response.status}):`, errorText);
                
                // Try to parse error as JSON for better error messages
                try {
                    const errorJson = JSON.parse(errorText);
                    throw new Error(errorJson.error || `HTTP ${response.status}`);
                } catch (parseError) {
                    throw new Error(`HTTP ${response.status}: ${errorText}`);
                }
            }
            
            // Try to parse success response
            let responseData = null;
            try {
                const responseText = await response.text();
                if (responseText.trim()) {
                    responseData = JSON.parse(responseText);
                    console.log('✓ Server response:', responseData);
                }
            } catch (parseError) {
                console.log('✓ Server responded successfully (no JSON data)');
            }
            
            showToast('success', 'Success', successMessage);
            return true; // Success
            
        } catch (error) {
            lastError = error;
            console.error(`Attempt ${retryCount + 1}/${maxRetries} failed:`, error.message);
            
            retryCount++;
            if (retryCount < maxRetries) {
                const delay = Math.min(Math.pow(2, retryCount) * 500, 3000); // Cap at 3s
                console.log(`Retrying in ${delay}ms...`);
                await new Promise(resolve => setTimeout(resolve, delay));
            }
        }
    }
    
    // All retries failed
    console.error('✗ Board request failed after all retries:', lastError?.message);
    showToast('error', 'Request Failed', `Failed to ${method.toLowerCase()} board configuration: ${lastError?.message}`);
    return false;
}

// Delete board configuration
async function deleteBoard(index) {
    // Confirm deletion
    if (!confirm('Are you sure you want to delete this board configuration?')) {
        return;
    }
    
    try {
        const boardId = boardConfigurations[index].id;
        console.log("Deleting board with ID:", boardId);
        
        const maxRetries = 3;
        let retryCount = 0;
        let lastError = null;
        
        while (retryCount < maxRetries) {
            try {
                console.log(`Attempt ${retryCount + 1}/${maxRetries} - Sending DELETE request`);
                
                // Add a cache-busting parameter
                const timestamp = new Date().getTime();
                const response = await fetch(`/api/boards/delete?id=${boardId}&t=${timestamp}`, {
                    method: 'GET',
                    headers: {
                        'Cache-Control': 'no-cache',
                        'Pragma': 'no-cache'
                    }
                });
                
                if (!response.ok) {
                    const errorText = await response.text();
                    console.error("Server response:", errorText);
                    throw new Error(`HTTP error! status: ${response.status}`);
                }
                
                // Success
                showToast('success', 'Success', 'Board deleted successfully');
                loadBoardConfigurations();
                return;
            } catch (error) {
                lastError = error;
                console.error(`Attempt ${retryCount + 1}/${maxRetries} failed:`, error);
                
                // Exponential backoff for retries
                retryCount++;
                if (retryCount < maxRetries) {
                    const delay = Math.pow(2, retryCount) * 500; // 1s, 2s, 4s, etc.
                    console.log(`Retrying in ${delay}ms...`);
                    await new Promise(resolve => setTimeout(resolve, delay));
                }
            }
        }
        
        // If we get here, all retries failed
        throw new Error(`Failed to delete board after ${maxRetries} attempts`);
    } catch (error) {
        console.error('Error deleting board:', error);
        showToast('error', 'Error', 'Failed to delete board');
    }
}

// Save board configuration
async function saveBoardConfiguration() {
    // Get form values
    const boardType = document.getElementById('boardType').value;
    const boardName = document.getElementById('boardName').value.trim();
    const modbusPort = document.getElementById('modbusPort').value;
    
    // Validate required fields
    if (!boardType || !boardName || !modbusPort) {
        showToast('error', 'Validation Error', 'Please fill in all required fields');
        return;
    }
    
    // Validate board name length (max 13 chars for NULL termination)
    if (boardName.length > 13) {
        showToast('error', 'Validation Error', 'Board name must be 13 characters or less');
        return;
    }
    
    // Validate specific board settings based on type
    let typeSpecificValidationPassed = true;
    let validationErrorMessage = '';

    // Validate poll time
    const pollTime = parseInt(document.getElementById('pollTime').value);
    if (isNaN(pollTime) || pollTime < 1 || pollTime > 3600) {
        typeSpecificValidationPassed = false;
        validationErrorMessage = 'Poll time must be between 1 second and 1 hour';
    }

    // Validate record interval
    const recordInterval = parseInt(document.getElementById('recordInterval').value);
    if (isNaN(recordInterval) || recordInterval < 15 || recordInterval > 3600) {
        typeSpecificValidationPassed = false;
        validationErrorMessage = 'Record interval must be between 15 seconds and 1 hour';
    }
    
    if (boardType === 'THERMOCOUPLE_IO') {
        // Check hysteresis values for all channels
        for (let i = 0; i < 8; i++) {
            const hysteresis = parseInt(document.getElementById(`alertHysteresis_${i}`).value);
            if (isNaN(hysteresis) || hysteresis < 0 || hysteresis > 255) {
                typeSpecificValidationPassed = false;
                validationErrorMessage = `Channel ${i+1}: Hysteresis must be between 0 and 255`;
                break;
            }
        }

        for (let i = 0; i < 8; i++) {
            const channelName = document.getElementById(`channelName_${i}`).value;
            if (!channelName) {
                typeSpecificValidationPassed = false;
                validationErrorMessage = `Channel ${i+1}: Custom channel name is required`;
                break;
            }
            else if (channelName.length > 32) {
                typeSpecificValidationPassed = false;
                validationErrorMessage = `Channel ${i+1}: Custom channel name must be 32 characters or less`;
                break;
            }
        }
    }
    
    if (!typeSpecificValidationPassed) {
        showToast('error', 'Validation Error', validationErrorMessage);
        return;
    }
    
    try {
        // Create board configuration object
        const boardData = {
            name: boardName,
            type: getBoardTypeValue(boardType), // Convert string type to enum value
            modbus_port: parseInt(modbusPort),
            poll_time: parseInt(document.getElementById('pollTime').value) * 1000,
            record_interval: parseInt(document.getElementById('recordInterval').value) * 1000
        };
        
        // Add board type specific settings
        if (boardType === 'THERMOCOUPLE_IO') {
            boardData.channels = [];
            
            // Add channel settings
            for (let i = 0; i < 8; i++) {
                const alertSetpoint = parseFloat(document.getElementById(`alertSetpoint_${i}`).value);
                const alertHysteresis = parseInt(document.getElementById(`alertHysteresis_${i}`).value);
                
                boardData.channels.push({
                    alert_enable: document.getElementById(`alertEnable_${i}`).checked,
                    output_enable: document.getElementById(`outputEnable_${i}`).checked,
                    alert_latch: document.getElementById(`alertLatch_${i}`).checked,
                    alert_edge: document.getElementById(`alertEdge_${i}`).checked,
                    tc_type: parseInt(document.getElementById(`tcType_${i}`).value),
                    alert_setpoint: alertSetpoint,
                    alert_hysteresis: alertHysteresis,
                    channel_name: document.getElementById(`channelName_${i}`).value,
                    record_temperature: document.getElementById(`recordTemperature_${i}`).checked,
                    record_cold_junction: document.getElementById(`recordColdJunction_${i}`).checked,
                    record_status: document.getElementById(`recordStatus_${i}`).checked,
                    show_on_dashboard: document.getElementById(`showOnDashboard_${i}`).checked,
                    monitor_fault: document.getElementById(`monitorFault_${i}`).checked,
                    monitor_alarm: document.getElementById(`monitorAlarm_${i}`).checked
                });
            }
        }

        if (editingBoardIndex !== null) {
            // Update existing board
            const boardId = boardConfigurations[editingBoardIndex].id;
            boardData.id = boardId;
            
            console.log("Sending update request:", JSON.stringify(boardData));
            const success = await sendBoardRequest('/api/boards', 'PUT', boardData, 'Board configuration updated');
            if (!success) return;
        } else {
            // Check board count limit before adding new board
            if (boardConfigurations.length >= 8) {
                showToast('error', 'Board Limit Reached', 'Maximum of 8 boards allowed. Please delete an existing board before adding a new one.');
                return;
            }
            
            // Add new board
            console.log("Sending add request:", JSON.stringify(boardData));
            
            // Convert numbers to ensure they're properly formatted
            if (boardData.type !== undefined) boardData.type = Number(boardData.type);
            if (boardData.modbus_port !== undefined) boardData.modbus_port = Number(boardData.modbus_port);
            if (boardData.poll_time !== undefined) boardData.poll_time = Number(boardData.poll_time);
            
            // Format channels data if present
            if (boardData.channels && Array.isArray(boardData.channels)) {
                boardData.channels.forEach(channel => {
                    if (channel.tc_type !== undefined) channel.tc_type = Number(channel.tc_type);
                    if (channel.alert_setpoint !== undefined) channel.alert_setpoint = Number(channel.alert_setpoint);
                    if (channel.alert_hysteresis !== undefined) channel.alert_hysteresis = Number(channel.alert_hysteresis);
                });
            }
            
            const jsonData = JSON.stringify(boardData);
            console.log("Formatted JSON data:", jsonData);
            
            const success = await sendBoardRequest('/api/boards', 'POST', boardData, 'New board added');
            if (!success) return;
        }
        
        // Hide form and refresh list
        hideBoardConfigForm();
        loadBoardConfigurations();
        
        // Reload dashboard items if board configuration has changed
        // as "show_on_dashboard" settings may have been updated
        if (document.querySelector('#dashboard').classList.contains('active')) {
            // If dashboard is the active tab, reload immediately
            loadDashboardItems();
        } else {
            // Flag that dashboard needs reloading when user switches to it
            window.dashboardNeedsReload = true;
        }
    } catch (error) {
        console.error('Error saving board configuration:', error);
        showToast('error', 'Error', 'Failed to save board configuration');
    }

    //connectStatusRefreshInterval = setInterval(updateBoardsListStatus, 2000);
}

// Render the list of configured boards - simplified version
function renderBoardsList() {
    const boardsList = document.getElementById('boardsList');
    const noBoards = document.getElementById('noBoards');
    
    if (!boardConfigurations || boardConfigurations.length === 0) {
        if (noBoards) noBoards.style.display = 'block';
        if (boardsList) {
            boardsList.innerHTML = '';
            boardsList.classList.remove('has-boards');
        }
        return;
    }
    
    if (noBoards) noBoards.style.display = 'none';
    if (boardsList) {
        boardsList.innerHTML = '';
        boardsList.classList.add('has-boards');
        
        // Render each board
        boardConfigurations.forEach((board, index) => {
            renderSingleBoard(board, index, boardsList);
        });
    }
}

// Simple function to render a single board in the list
function renderSingleBoard(board, index, container) {
    // Create board item element
    const boardItem = document.createElement('div');
    boardItem.className = 'board-item';
    
    // Determine board type name
    let boardTypeName = 'Unknown';
    if (board.type_name) {
        boardTypeName = board.type_name;
    } else if (board.type === 0) {
        boardTypeName = 'Thermocouple IO';
    }
    
    // Determine initialisation status
    const initStatus = board.initialised ? 'Initialised' : 'Not Initialised';
    const initStatusClass = initStatus === 'Initialised' ? 'init-status-ok' : 'init-status-pending';
    
    // Determine connection status
    const connectStatus = board.connected ? 'Online' : 'Offline';
    const connectStatusClass = connectStatus === 'Online' ? 'connect-status-ok' : 'connect-status-error';
    
    // Create the board item HTML
    boardItem.innerHTML = `
        <div class="board-info">
        <span class="board-type">${board.name || 'Unnamed Board'}</span>
        <span class="board-details">Type: ${boardTypeName}, Port: ${board.modbus_port + 1}, Slave ID: ${board.slave_id}</span>
        <div class="board-status-container">
            <span class="board-init-status ${initStatusClass}" id="init-status-${index}">Status: ${initStatus}</span>
            <span class="board-connect-status ${connectStatusClass}" id="connect-status-${index}">Connection: ${connectStatus}</span>
        </div>
    </div>
        <div class="board-actions">
            <button class="btn-edit" data-index="${index}"><i class="fas fa-edit"></i> Edit</button>
            <button class="btn-initialise" data-index="${index}" onclick="console.log('Init button direct click for index ${index}');showInitialisePrompt(${index})"><i class="fas fa-microchip"></i> Initialise</button>
            <button class="btn-delete" data-index="${index}"><i class="fas fa-trash"></i> Delete</button>
        </div>
    `;
    
    // Add to container
    container.appendChild(boardItem);
    
    // Add event listeners
    const editBtn = boardItem.querySelector('.btn-edit');
    if (editBtn) {
        editBtn.addEventListener('click', () => editBoard(index));
    }
    
    // We're still adding this event listener, but also added the onclick attribute for direct testing
    const initBtn = boardItem.querySelector('.btn-initialise');
    if (initBtn) {
        console.log(`Adding click handler to initialise button for board index ${index}`);
        initBtn.addEventListener('click', function() {
            console.log(`Initialise button clicked for board index ${index}`);
            showInitialisePrompt(index);
        });
    }
    
    const deleteBtn = boardItem.querySelector('.btn-delete');
    if (deleteBtn) {
        deleteBtn.addEventListener('click', () => deleteBoard(index));
    }
}

function updateBoardsListStatus() {
    if (document.getElementById('board-config').classList.contains('active')) {
        loadBoardsListStatus();
    }
    // Clear any existing interval
    if (connectStatusRefreshInterval) {
        clearInterval(connectStatusRefreshInterval);
    }

    //connectStatusRefreshInterval = setInterval(updateBoardsListStatus, 2000);
}

async function loadBoardsListStatus() {
    console.log('Updating boards list connection status...');
    try {        
        const response = await fetch(`/api/status`);
        
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const boards = await response.json();
        console.log('Board connection status data:', boards);
        
        // Update connection and initialisation status
        const connectStatus = boards.map(board => board.connected);
        const initStatus = boards.map(board => board.initialised);
        
        // Update connection and initialisation status for each board
        for (let i = 0; i < boardConfigurations.length; i++) {
            const initStatusElement = document.getElementById(`init-status-${i}`);
            if (initStatusElement) {
                const isInitialised = initStatus[i];
                initStatusElement.textContent = `Status: ${isInitialised ? 'Initialised' : 'Not Initialised'}`;
                initStatusElement.className = `board-init-status ${isInitialised ? 'init-status-ok' : 'init-status-pending'}`;
            }

            const connectStatusElement = document.getElementById(`connect-status-${i}`);
            if (connectStatusElement) {
                const isConnected = connectStatus[i];
                connectStatusElement.textContent = `Connection: ${isConnected ? 'Online' : 'Offline'}`;
                connectStatusElement.className = `board-connect-status ${isConnected ? 'connect-status-ok' : 'connect-status-error'}`;
            }
        }
    } catch (error) {
        console.error('Error loading board connection status:', error);
        showToast('error', 'Error', 'Failed to load board connection status information');
    }
}

// Edit board configuration
function editBoard(index) {
    showBoardConfigForm(index);
}

// Restore the board configuration form functionality
// Show board configuration form
function showBoardConfigForm(editIndex = null) {
    const boardConfigForm = document.getElementById('boardConfigForm');
    const formTitle = document.getElementById('formTitle');
    
    if (editIndex !== null) {
        console.log(`Editing board index ${editIndex}`);
        // Edit existing board
        editingBoardIndex = editIndex;
        const board = boardConfigurations[editIndex];
        
        if (formTitle) {
            formTitle.textContent = 'Edit Expansion Board';
        }
        
        // Fill form with board data
        const typeSelectElement = document.getElementById('boardType');
        // Convert numeric type to string type for the dropdown
        switch (board.type) {
            case 0:
                typeSelectElement.value = 'CONTROLLER_IO';
                break;
            case 1:
                typeSelectElement.value = 'ANALOGUE_DIGITAL_IO';
                break;
            case 2:
                typeSelectElement.value = 'THERMOCOUPLE_IO';
                break;
            case 3:
                typeSelectElement.value = 'RTD_IO';
                break;
            case 4:
                typeSelectElement.value = 'ENERGY_METER';
                break;
            default:
                typeSelectElement.value = 'THERMOCOUPLE_IO'; // Default to thermocouple
        }
        
        document.getElementById('boardName').value = board.name || '';
        document.getElementById('modbusPort').value = board.modbus_port;
        document.getElementById('pollTime').value = board.poll_time / 1000;
        document.getElementById('recordInterval').value = board.record_interval / 1000;
        
        // Show board type specific settings
        showBoardTypeSettings(typeSelectElement.value);
        
        // Fill board type specific settings
        if (board.type === 2) { // THERMOCOUPLE_IO
            // Fill channel settings
            if (board.channels && Array.isArray(board.channels)) {
                board.channels.forEach((channel, index) => {
                    document.getElementById(`alertEnable_${index}`).checked = channel.alert_enable;
                    document.getElementById(`outputEnable_${index}`).checked = channel.output_enable;
                    document.getElementById(`alertLatch_${index}`).checked = channel.alert_latch;
                    document.getElementById(`alertEdge_${index}`).checked = channel.alert_edge;
                    document.getElementById(`tcType_${index}`).value = channel.tc_type;
                    document.getElementById(`alertSetpoint_${index}`).value = channel.alert_setpoint;
                    document.getElementById(`alertHysteresis_${index}`).value = channel.alert_hysteresis;
                    document.getElementById(`channelName_${index}`).value = channel.channel_name;
                    document.getElementById(`recordTemperature_${index}`).checked = channel.record_temperature;
                    document.getElementById(`recordColdJunction_${index}`).checked = channel.record_cold_junction;
                    document.getElementById(`recordStatus_${index}`).checked = channel.record_status;
                    document.getElementById(`showOnDashboard_${index}`).checked = channel.show_on_dashboard;
                    document.getElementById(`monitorFault_${index}`).checked = channel.monitor_fault;
                    document.getElementById(`monitorAlarm_${index}`).checked = channel.monitor_alarm;
                });
            }
        }
        // Add more board types as needed
    } else {
        console.log("Adding new board");
        // Add new board
        editingBoardIndex = null;
        
        if (formTitle) {
            formTitle.textContent = 'Add New Expansion Board';
        }
        
        // Reset form
        document.getElementById('boardType').value = 'THERMOCOUPLE_IO';
        document.getElementById('boardName').value = '';
        document.getElementById('modbusPort').value = '0';
        document.getElementById('pollTime').value = '15';
        document.getElementById('recordInterval').value = '15';
        
        // Show default board type settings
        showBoardTypeSettings('THERMOCOUPLE_IO');
    }
    
    if (boardConfigForm) {
        boardConfigForm.style.display = 'block';
        
        // Scroll to the board configuration form for better UX
        setTimeout(() => {
            const formTitle = document.getElementById('formTitle');
            if (formTitle) {
                formTitle.scrollIntoView({ 
                    behavior: 'smooth', 
                    block: 'start',
                    inline: 'nearest'
                });
            }
        }, 100); // Small delay to ensure form is fully rendered
    }
}

// Hide board configuration form
function hideBoardConfigForm() {
    const boardConfigForm = document.getElementById('boardConfigForm');
    
    if (boardConfigForm) {
        boardConfigForm.style.display = 'none';
    }
    
    // Reset form state
    editingBoardIndex = null;
}

// Show duplicate board selection modal
function showDuplicateBoardModal() {
    const duplicateBoardModal = document.getElementById('duplicateBoardModal');
    const boardToDuplicate = document.getElementById('boardToDuplicate');
    const duplicatedBoardName = document.getElementById('duplicatedBoardName');
    
    // Check if there are any boards to duplicate
    if (!boardConfigurations || boardConfigurations.length === 0) {
        showToast('warning', 'No Boards Available', 'No boards are configured to duplicate.');
        return;
    }
    
    // Check if we've reached the maximum number of boards
    if (boardConfigurations.length >= 8) {
        showToast('warning', 'Board Limit Reached', 'Maximum number of boards (8) already configured.');
        return;
    }
    
    // Populate the board selection dropdown
    boardToDuplicate.innerHTML = '';
    boardConfigurations.forEach((board, index) => {
        const option = document.createElement('option');
        option.value = index;
        option.textContent = `${board.name || `Board ${index + 1}`} (${getBoardTypeString(board.type)})`;
        boardToDuplicate.appendChild(option);
    });
    
    // Clear the new board name field
    duplicatedBoardName.value = '';
    
    // Show the modal
    duplicateBoardModal.style.display = 'block';
}

// Duplicate the selected board
function duplicateSelectedBoard() {
    const boardToDuplicate = document.getElementById('boardToDuplicate');
    const duplicatedBoardName = document.getElementById('duplicatedBoardName');
    const duplicateBoardModal = document.getElementById('duplicateBoardModal');
    
    const sourceIndex = parseInt(boardToDuplicate.value);
    const newBoardName = duplicatedBoardName.value.trim();
    
    // Validate inputs
    if (isNaN(sourceIndex) || sourceIndex < 0 || sourceIndex >= boardConfigurations.length) {
        showToast('error', 'Invalid Selection', 'Please select a valid board to duplicate.');
        return;
    }
    
    if (!newBoardName) {
        showToast('error', 'Name Required', 'Please enter a name for the duplicated board.');
        return;
    }
    
    // Check if name already exists
    const nameExists = boardConfigurations.some(board => board.name === newBoardName);
    if (nameExists) {
        showToast('error', 'Name Already Exists', 'A board with this name already exists. Please choose a different name.');
        return;
    }
    
    // Create a deep copy of the source board
    const sourceBoard = boardConfigurations[sourceIndex];
    const duplicatedBoard = JSON.parse(JSON.stringify(sourceBoard));
    
    // Update the duplicated board's name and keep the same modbus port
    duplicatedBoard.name = newBoardName;
    // Keep the same Modbus Port as the source board since multiple boards can share the same RS485 bus
    
    console.log('Duplicating board:', sourceBoard.name, 'with port:', sourceBoard.modbus_port);
    console.log('New board:', newBoardName, 'keeping same port:', duplicatedBoard.modbus_port);
    
    // Hide the modal
    duplicateBoardModal.style.display = 'none';
    
    // Show the board configuration form with the duplicated data
    showBoardConfigFormWithData(duplicatedBoard);
    
    showToast('success', 'Board Duplicated', `Board configuration copied successfully. Please review and save the new board.`);
}

// Note: getNextAvailableModbusPort() function removed since Modbus Port represents
// the RS485 bus (0 or 1) and multiple boards can share the same bus.
// When duplicating boards, we keep the same Modbus Port as the source board.

// Get board type string for display
function getBoardTypeString(type) {
    switch (type) {
        case 0: return 'Controller IO';
        case 1: return 'Analogue/Digital IO';
        case 2: return 'Thermocouple IO';
        case 3: return 'RTD IO';
        case 4: return 'Energy Meter';
        default: return 'Unknown';
    }
}

// Show board configuration form with pre-filled data
function showBoardConfigFormWithData(boardData) {
    const boardConfigForm = document.getElementById('boardConfigForm');
    const formTitle = document.getElementById('formTitle');
    
    // Set form to add mode (not editing existing board)
    editingBoardIndex = null;
    
    if (formTitle) {
        formTitle.textContent = 'Add New Expansion Board (Duplicated)';
    }
    
    // Fill form with duplicated board data
    const typeSelectElement = document.getElementById('boardType');
    // Convert numeric type to string type for the dropdown
    switch (boardData.type) {
        case 0:
            typeSelectElement.value = 'CONTROLLER_IO';
            break;
        case 1:
            typeSelectElement.value = 'ANALOGUE_DIGITAL_IO';
            break;
        case 2:
            typeSelectElement.value = 'THERMOCOUPLE_IO';
            break;
        case 3:
            typeSelectElement.value = 'RTD_IO';
            break;
        case 4:
            typeSelectElement.value = 'ENERGY_METER';
            break;
        default:
            typeSelectElement.value = 'THERMOCOUPLE_IO';
    }
    
    document.getElementById('boardName').value = boardData.name || '';
    console.log('Setting modbus port field to:', boardData.modbus_port, 'type:', typeof boardData.modbus_port);
    document.getElementById('modbusPort').value = boardData.modbus_port;
    document.getElementById('pollTime').value = boardData.poll_time / 1000;
    document.getElementById('recordInterval').value = boardData.record_interval / 1000;
    
    // Show board type specific settings
    showBoardTypeSettings(typeSelectElement.value);
    
    // Fill board type specific settings
    if (boardData.type === 2) { // THERMOCOUPLE_IO
        // Fill channel settings
        if (boardData.channels && Array.isArray(boardData.channels)) {
            boardData.channels.forEach((channel, index) => {
                document.getElementById(`alertEnable_${index}`).checked = channel.alert_enable;
                document.getElementById(`outputEnable_${index}`).checked = channel.output_enable;
                document.getElementById(`alertLatch_${index}`).checked = channel.alert_latch;
                document.getElementById(`alertEdge_${index}`).checked = channel.alert_edge;
                document.getElementById(`tcType_${index}`).value = channel.tc_type;
                document.getElementById(`alertSetpoint_${index}`).value = channel.alert_setpoint;
                document.getElementById(`alertHysteresis_${index}`).value = channel.alert_hysteresis;
                document.getElementById(`channelName_${index}`).value = channel.channel_name;
                document.getElementById(`recordTemperature_${index}`).checked = channel.record_temperature;
                document.getElementById(`recordColdJunction_${index}`).checked = channel.record_cold_junction;
                document.getElementById(`recordStatus_${index}`).checked = channel.record_status;
                document.getElementById(`showOnDashboard_${index}`).checked = channel.show_on_dashboard;
                document.getElementById(`monitorFault_${index}`).checked = channel.monitor_fault;
                document.getElementById(`monitorAlarm_${index}`).checked = channel.monitor_alarm;
            });
        }
    }
    // Add more board types as needed
    
    if (boardConfigForm) {
        boardConfigForm.style.display = 'block';
        
        // Scroll to the board configuration form for better UX
        setTimeout(() => {
            const formTitle = document.getElementById('formTitle');
            if (formTitle) {
                formTitle.scrollIntoView({ 
                    behavior: 'smooth', 
                    block: 'start',
                    inline: 'nearest'
                });
            }
        }, 100);
    }
}

// Show settings specific to the selected board type
function showBoardTypeSettings(boardType) {
    // Hide all board type settings
    document.querySelectorAll('.board-type-settings').forEach(settings => {
        settings.style.display = 'none';
    });
    
    // Show selected board type settings
    switch (boardType) {
        case 'THERMOCOUPLE_IO':
            const thermocoupleSettings = document.getElementById('thermocoupleSettings');
            if (thermocoupleSettings) {
                thermocoupleSettings.style.display = 'block';
                setupThermocoupleChannelTabs();
            }
            break;
        // Add more board types as they're supported
    }
}

// Set up channel tabs for thermocouple IO board
function setupThermocoupleChannelTabs() {
    const tabHeaders = document.querySelector('.channel-tab-headers');
    const tabContent = document.querySelector('.channel-tab-content');
    
    if (!tabHeaders || !tabContent) return;
    
    // Clear existing content
    tabHeaders.innerHTML = '';
    tabContent.innerHTML = '';
    
    // Create tabs and content for 8 channels
    for (let i = 0; i < 8; i++) {
        // Create tab header
        const tabHeader = document.createElement('div');
        tabHeader.className = i === 0 ? 'channel-tab active' : 'channel-tab';
        tabHeader.dataset.channel = i;
        tabHeader.textContent = `Channel ${i + 1}`;
        tabHeaders.appendChild(tabHeader);
        
        // Create tab content
        const content = document.createElement('div');
        content.className = i === 0 ? 'channel-content active' : 'channel-content';
        content.dataset.channel = i;
        
        content.innerHTML = `
            <div class="channel-form">
                <div class="channel-form-left">
                    <h4>Configuration switches</h4>
                    <div class="checkbox-group">
                        <input type="checkbox" id="alertEnable_${i}" class="form-check">
                        <label for="alertEnable_${i}">Enable temperature alert</label>
                    </div>
                    <div class="checkbox-group">
                        <input type="checkbox" id="outputEnable_${i}" class="form-check">
                        <label for="outputEnable_${i}">Enable output</label>
                    </div>
                    <div class="checkbox-group">
                        <input type="checkbox" id="alertLatch_${i}" class="form-check">
                        <label for="alertLatch_${i}">Alert is latched until cleared</label>
                    </div>
                    <div class="checkbox-group">
                        <input type="checkbox" id="alertEdge_${i}" class="form-check">
                        <label for="alertEdge_${i}">Alert on falling temperature</label>
                    </div>
                    <div class="checkbox-group">
                        <input type="checkbox" id="monitorFault_${i}" class="form-check">
                        <label for="monitorFault_${i}">Monitor faults</label>
                    </div>
                    <div class="checkbox-group">
                        <input type="checkbox" id="monitorAlarm_${i}" class="form-check">
                        <label for="monitorAlarm_${i}">Monitor alarms</label>
                    </div>
                </div>
                <div class="channel-form-centre">
                    <h4>Thermocouple settings</h4>
                    <div class="form-group">
                        <label for="tcType_${i}">Thermocouple Type:</label>
                        <select id="tcType_${i}" class="form-control">
                            <option value="0">Type K</option>
                            <option value="1">Type J</option>
                            <option value="2">Type T</option>
                            <option value="3">Type N</option>
                            <option value="4">Type S</option>
                            <option value="5">Type E</option>
                            <option value="6">Type B</option>
                            <option value="7">Type R</option>
                        </select>
                    </div>
                    <div class="form-group">
                        <label for="alertSetpoint_${i}">Alert Setpoint (°C):</label>
                        <input type="number" id="alertSetpoint_${i}" class="form-control" step="0.1" value="0">
                    </div>
                    <div class="form-group">
                        <label for="alertHysteresis_${i}">Alert Hysteresis (°C):</label>
                        <input type="number" id="alertHysteresis_${i}" class="form-control" min="0" max="255" step="1" value="0">
                        <small class="helper-text"> Range: 0-255</small>
                    </div>
                    <div class="form-group">
                        <label for="channelName_${i}">Custom channel name:</label>
                        <input type="text" id="channelName_${i}" class="form-control" value="Channel ${i + 1}">
                    </div>
                </div>
                <div class="channel-form-right">
                    <h4>Record & display settings</h4>
                    <div class="checkbox-group">
                        <input type="checkbox" id="recordTemperature_${i}" class="form-check">
                        <label for="recordTemperature_${i}">Record temperature</label>
                    </div>
                    <div class="checkbox-group">
                        <input type="checkbox" id="recordColdJunction_${i}" class="form-check">
                        <label for="recordColdJunction_${i}">Record cold junction</label>
                    </div>
                    <div class="checkbox-group">
                        <input type="checkbox" id="recordStatus_${i}" class="form-check">
                        <label for="recordStatus_${i}">Record input status</label>
                    </div>
                    <div class="checkbox-group">
                        <input type="checkbox" id="showOnDashboard_${i}" class="form-check">
                        <label for="showOnDashboard_${i}">Show on dashboard</label>
                    </div>
                </div>
            </div>
        `;
        
        tabContent.appendChild(content);
    }
    
    // Add tab switching functionality
    document.querySelectorAll('.channel-tab').forEach(tab => {
        tab.addEventListener('click', (e) => {
            const channelIndex = e.target.dataset.channel;
            
            // Update active tab
            document.querySelectorAll('.channel-tab').forEach(t => {
                t.classList.remove('active');
            });
            e.target.classList.add('active');
            
            // Update active content
            document.querySelectorAll('.channel-content').forEach(c => {
                c.classList.remove('active');
            });
            document.querySelector(`.channel-content[data-channel="${channelIndex}"]`).classList.add('active');
        });
    });
}

document.addEventListener('DOMContentLoaded', () => {
    console.log('DOM loaded - initialising board configuration system');
    
    // Add tab switching behavior for Board Configuration tab
    const boardConfigTab = document.querySelector('a[data-page="board-config"]');
    if (boardConfigTab) {
        boardConfigTab.addEventListener('click', () => {
            console.log('Board config tab clicked - refreshing board configurations');
            loadBoardConfigurations();
        });
    }
    
    // Add board button click handler
    const addBoardBtn = document.getElementById('addBoardBtn');
    if (addBoardBtn) {
        addBoardBtn.addEventListener('click', () => {
            showBoardConfigForm();
        });
    }
    
    // Export config button click handler
    const exportConfigBtn = document.getElementById('exportConfigBtn');
    if (exportConfigBtn) {
        exportConfigBtn.addEventListener('click', () => {
            exportBoardConfiguration();
        });
    }
    
    // Import config button click handler
    const importConfigBtn = document.getElementById('importConfigBtn');
    if (importConfigBtn) {
        importConfigBtn.addEventListener('click', () => {
            document.getElementById('configFileInput').click();
        });
    }
    
    // Duplicate board button click handler
    const duplicateBoardBtn = document.getElementById('duplicateBoardBtn');
    if (duplicateBoardBtn) {
        duplicateBoardBtn.addEventListener('click', () => {
            showDuplicateBoardModal();
        });
    }
    
    // File input change handler for config import
    const configFileInput = document.getElementById('configFileInput');
    if (configFileInput) {
        configFileInput.addEventListener('change', (e) => {
            importBoardConfiguration(e.target.files[0]);
        });
    }

    // Enhanced import confirmation modal handlers
    const importConfirmModal = document.getElementById('importConfirmModal');
    const cancelImport = document.getElementById('cancelImport');
    const importOverwrite = document.getElementById('importOverwrite');
    const importOnline = document.getElementById('importOnline');
    const closeImportModal = importConfirmModal?.querySelector('.close');

    // Cancel import handler
    if (cancelImport) {
        cancelImport.addEventListener('click', () => {
            importConfirmModal.style.display = 'none';
            // Reset the file input
            document.getElementById('configFileInput').value = '';
            window.selectedConfigFile = null;
        });
    }

    // Import and Overwrite handler (original behavior)
    if (importOverwrite) {
        importOverwrite.addEventListener('click', () => {
            importConfirmModal.style.display = 'none';
            // Proceed with import using the stored file (boards will be uninitialized)
            if (window.selectedConfigFile) {
                performImport(window.selectedConfigFile, 'overwrite');
                window.selectedConfigFile = null;
            }
        });
    }

    // Import and Online handler (new behavior - set boards as initialized)
    if (importOnline) {
        importOnline.addEventListener('click', () => {
            importConfirmModal.style.display = 'none';
            // Proceed with import and set boards as initialized
            if (window.selectedConfigFile) {
                performImport(window.selectedConfigFile, 'online');
                window.selectedConfigFile = null;
            }
        });
    }

    // Close modal handler
    if (closeImportModal) {
        closeImportModal.addEventListener('click', () => {
            importConfirmModal.style.display = 'none';
            // Reset the file input
            document.getElementById('configFileInput').value = '';
            window.selectedConfigFile = null;
        });
    }

    // Close the modal when clicking outside it
    if (importConfirmModal) {
        window.addEventListener('click', (event) => {
            if (event.target === importConfirmModal) {
                importConfirmModal.style.display = 'none';
                // Reset the file input
                document.getElementById('configFileInput').value = '';
                window.selectedConfigFile = null;
            }
        });
    }
    
    // Duplicate board modal handlers
    const duplicateBoardModal = document.getElementById('duplicateBoardModal');
    const cancelDuplicate = document.getElementById('cancelDuplicate');
    const confirmDuplicate = document.getElementById('confirmDuplicate');
    const closeDuplicateModal = document.getElementById('closeDuplicateModal');

    if (cancelDuplicate) {
        cancelDuplicate.addEventListener('click', () => {
            duplicateBoardModal.style.display = 'none';
        });
    }

    if (confirmDuplicate) {
        confirmDuplicate.addEventListener('click', () => {
            duplicateSelectedBoard();
        });
    }

    if (closeDuplicateModal) {
        closeDuplicateModal.addEventListener('click', () => {
            duplicateBoardModal.style.display = 'none';
        });
    }

    // Close the duplicate modal when clicking outside it
    if (duplicateBoardModal) {
        window.addEventListener('click', (event) => {
            if (event.target === duplicateBoardModal) {
                duplicateBoardModal.style.display = 'none';
            }
        });
    }
    
    // Board type change handler
    const boardTypeSelect = document.getElementById('boardType');
    if (boardTypeSelect) {
        boardTypeSelect.addEventListener('change', () => {
            showBoardTypeSettings(boardTypeSelect.value);
        });
    }
    
    // Cancel button click handler
    const cancelBoardConfig = document.getElementById('cancelBoardConfig');
    if (cancelBoardConfig) {
        cancelBoardConfig.addEventListener('click', () => {
            hideBoardConfigForm();
        });
    }
    
    // Save button click handler
    const saveBoardConfig = document.getElementById('saveBoardConfig');
    if (saveBoardConfig) {
        saveBoardConfig.addEventListener('click', () => {
            saveBoardConfiguration();
        });
    }
    
    // Set up board initialisation handlers - with direct function call for debugging
    const cancelInitBoard = document.getElementById('cancelInitBoard');
    const confirmInitBoard = document.getElementById('confirmInitBoard');
    const initBoardModal = document.getElementById('initBoardModal');
    
    console.log('Setting up initialisation modal handlers');
    console.log('Cancel button exists:', !!cancelInitBoard);
    console.log('Confirm button exists:', !!confirmInitBoard);
    console.log('Modal exists:', !!initBoardModal);
    
    if (cancelInitBoard) {
        console.log('Setting up cancel initialisation handler');
        cancelInitBoard.addEventListener('click', () => {
            console.log('Cancel initialisation clicked');
            hideInitialisePrompt();
        });
    } else {
        console.log('Cancel initialisation button not found');
    }
    
    if (confirmInitBoard) {
        console.log('Setting up confirm initialisation handler');
        confirmInitBoard.onclick = function() {
            console.log('Confirm initialisation clicked via onclick');
            
            // Store the index in a local variable before it gets reset
            const boardIndexToInitialise = initialisingBoardIndex;
            console.log('Stored board index for initialisation:', boardIndexToInitialise);
            
            // Hide the prompt (this will set initialisingBoardIndex to null)
            hideInitialisePrompt();
            
            // Initialise the board using our locally stored index
            if (boardIndexToInitialise !== null) {
                console.log('Initialising board index:', boardIndexToInitialise);
                
                // Use a setTimeout to allow the modal to be hidden first
                setTimeout(() => {
                    console.log('Calling initialiseBoard with index:', boardIndexToInitialise);
                    initialiseBoard(boardIndexToInitialise);
                }, 100);
            } else {
                console.error('No board index specified for initialisation');
            }
        };
    } else {
        console.log('Confirm initialisation button not found');
    }
    
    // Close the modal when clicking outside it
    if (initBoardModal) {
        console.log('Setting up modal outside click handler');
        initBoardModal.addEventListener('click', (e) => {
            if (e.target === initBoardModal) {
                hideInitialisePrompt();
            }
        });
    } else {
        console.log('Modal not found');
    }
});

// Global variable to store the current board being initialised
let initialisingBoardIndex = null;

// Show the board initialisation prompt
function showInitialisePrompt(index) {
    console.log('showInitialisePrompt called with index:', index);
    
    // Store the board index
    initialisingBoardIndex = index;
    
    // Show the modal - with additional debugging
    const initBoardModal = document.getElementById('initBoardModal');
    console.log('Modal element:', initBoardModal);
    
    if (initBoardModal) {
        console.log('Found initBoardModal, adding active class');
        initBoardModal.classList.add('active');
        console.log('Current classes:', initBoardModal.className);
        
        // Double-check that our buttons exist
        const cancelBtn = document.getElementById('cancelInitBoard');
        const confirmBtn = document.getElementById('confirmInitBoard');
        console.log('Cancel button exists:', !!cancelBtn);
        console.log('Confirm button exists:', !!confirmBtn);
    } else {
        console.error('ERROR: Could not find initBoardModal element in the DOM');
        // List all modal-overlay elements to see what exists
        const allModals = document.querySelectorAll('.modal-overlay');
        console.log('Found', allModals.length, 'modal-overlay elements');
        allModals.forEach((modal, i) => {
            console.log(`Modal ${i} id:`, modal.id);
        });
    }
}

// Hide the board initialisation prompt
function hideInitialisePrompt() {
    console.log('hideInitialisePrompt called');
    
    // Reset the board index
    initialisingBoardIndex = null;
    
    // Hide the modal
    const initBoardModal = document.getElementById('initBoardModal');
    if (initBoardModal) {
        console.log('Found initBoardModal, removing active class');
        initBoardModal.classList.remove('active');
    } else {
        console.error('ERROR: Could not find initBoardModal element when trying to hide it');
    }
}

// Initialise a board by assigning it an address - with direct XHR for better debug visibility
async function initialiseBoard(index) {
    if (index === null || index < 0 || index >= boardConfigurations.length) {
        showToast('error', 'Error', 'Invalid board index for initialisation');
        return;
    }
    
    showToast('info', 'Initialising...', 'Attempting to initialise board. This may take a few seconds.', 10000);
    console.log('Attempting to initialise board at index:', index);
    
    try {
        // Use XMLHttpRequest for better debug visibility
        const xhr = new XMLHttpRequest();
        const url = `/api/boards/initialise?id=${index}`;
        
        console.log('Making XHR request to:', url);
        
        // Enable CORS and credentials
        xhr.withCredentials = true;
        
        // Log all XHR events
        xhr.onloadstart = () => console.log('XHR load started');
        xhr.onprogress = (e) => console.log(`XHR progress: ${e.loaded} bytes`);
        xhr.onabort = () => console.log('XHR aborted');
        xhr.onerror = () => console.log('XHR error occurred');
        xhr.ontimeout = () => console.log('XHR timed out');
        xhr.onloadend = () => console.log('XHR load ended');
        
        // Return a promise that resolves when the request completes
        const response = await new Promise((resolve, reject) => {
            xhr.onreadystatechange = function() {
                console.log(`XHR state change: readyState=${xhr.readyState}, status=${xhr.status}`);
                
                if (xhr.readyState === 4) {
                    if (xhr.status >= 200 && xhr.status < 300) {
                        try {
                            const result = JSON.parse(xhr.responseText);
                            resolve({ ok: true, status: xhr.status, result });
                        } catch (parseError) {
                            console.error('Error parsing JSON response:', parseError);
                            reject(new Error('Invalid JSON response'));
                        }
                    } else {
                        console.error('XHR request failed:', xhr.status, xhr.statusText);
                        console.error('XHR response text:', xhr.responseText);
                        reject(new Error(`Server returned ${xhr.status}: ${xhr.statusText}`));
                    }
                }
            };
            
            // Set a slightly longer timeout for initialization
            xhr.timeout = 15000; // 15 seconds
            
            xhr.open('GET', url, true);
            
            // Add explicit CORS headers
            xhr.setRequestHeader('Access-Control-Allow-Origin', '*');
            xhr.setRequestHeader('Access-Control-Allow-Methods', 'GET');
            xhr.setRequestHeader('Access-Control-Allow-Headers', 'Content-Type');
            
            xhr.send();
            console.log('XHR request sent');
        });
        
        console.log('XHR response:', response);
        
        // Remove the initialising toast
        document.querySelectorAll('.toast').forEach(toast => {
            if (toast.querySelector('.toast-title').textContent === 'Initialising...') {
                toast.remove();
            }
        });
        
        if (!response.ok) {
            throw new Error(response.result.error || 'Failed to initialise board');
        }
        
        // Show success message
        showToast('success', 'Success', `Board ${boardConfigurations[index].name} initialised with Slave ID ${response.result.slave_id}`);
        
        // Update the board configuration in our local array
        boardConfigurations[index].initialised = true;
        boardConfigurations[index].slave_id = response.result.slave_id;
        
        // Re-render the board list to show the updated initialisation status
        renderBoardsList();
        
    } catch (error) {
        console.error('Board initialisation error:', error);
        
        // Show error message
        showToast('error', 'Error', `Failed to initialise board. ${error.message || ''} Ensure the board is in Address Assignment Mode (blue LED lit).`);
    }
}

// Board Status Page Functionality
let boardStatusChart = null;
let selectedBoardId = null;
let statusRefreshInterval = null;
let connectStatusRefreshInterval = null;

// Client-side data storage for temperature history (replacing server-side history)
const CLIENT_HISTORY_MAX_POINTS = 900; // Maximum number of data points to store (900 points = 30 minutes with 2s refresh)
let clientTemperatureHistory = {
    timestamps: [],
    channels: []
};

// Chart colors for different channels
const chartColors = [
    'rgb(54, 162, 235)',  // Blue
    'rgb(255, 99, 132)',  // Red
    'rgb(255, 205, 86)',  // Yellow
    'rgb(75, 192, 192)',  // Teal
    'rgb(153, 102, 255)', // Purple
    'rgb(255, 159, 64)',  // Orange
    'rgb(201, 203, 207)', // Grey
    'rgb(99, 255, 132)'   // Green
];

// Lookup table for thermocouple types
const thermocoupleLookup = {
    0: 'K',
    1: 'J',
    2: 'T',
    3: 'E',
    4: 'N',
    5: 'S',
    6: 'B',
    7: 'R'
};

// Initialize the board status page
function initBoardStatusPage() {
    console.log('Initializing board status page');
    
    // Clear any existing interval
    if (statusRefreshInterval) {
        clearInterval(statusRefreshInterval);
    }
    
    // Load the board list
    loadBoardStatusList();
    
    // Set up the board selector change event
    const boardSelector = document.getElementById('statusBoardSelect');
    if (boardSelector) {
        boardSelector.addEventListener('change', function() {
            selectedBoardId = this.value;
            if (selectedBoardId) {
                loadBoardStatus(selectedBoardId);
            }
        });
    }
    
    // Start the refresh interval (every 2 seconds)
    statusRefreshInterval = setInterval(refreshBoardStatus, 2000);
}

// Load the list of boards for the status page
async function loadBoardStatusList() {
    try {
        const response = await fetch('/api/status');
        
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const boards = await response.json();
        const boardSelector = document.getElementById('statusBoardSelect');
        const noBoardsMessage = document.getElementById('noBoardsStatus');
        const boardStatusContent = document.getElementById('boardStatusContent');
        
        // Clear existing options
        boardSelector.innerHTML = '';
        
        // Check if we have boards
        if (boards && boards.length > 0) {
            // Add options for each board (including disconnected ones)
            boards.forEach(board => {
                const option = document.createElement('option');
                option.value = board.id;
                const statusText = board.connected ? '' : ' - DISCONNECTED';
                option.textContent = `${board.name} (${board.type})${statusText}`;
                if (!board.connected) {
                    option.style.color = '#ff6b6b'; // Red color for disconnected boards
                    option.style.fontWeight = 'bold';
                }
                boardSelector.appendChild(option);
            });
            
            // Hide no boards message
            noBoardsMessage.style.display = 'none';
            
            // If we have at least one board, select it and load its status
            if (boardSelector.options.length > 0) {
                boardSelector.selectedIndex = 0;
                selectedBoardId = boardSelector.value;
                boardStatusContent.style.display = 'block';
                loadBoardStatus(selectedBoardId);
            } else {
                // No connected boards
                noBoardsMessage.style.display = 'block';
                boardStatusContent.style.display = 'none';
            }
        } else {
            // No boards configured
            noBoardsMessage.style.display = 'block';
            boardStatusContent.style.display = 'none';
        }
    } catch (error) {
        console.error('Error loading board status list:', error);
        showToast('error', 'Error', 'Failed to load board list');
    }
}

// Load the status of a specific board
async function loadBoardStatus(boardId) {
    try {        
        const response = await fetch(`/api/status/board?id=${boardId}`);
        
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const board = await response.json();
        console.log('Board status data:', board); // Debug the full API response
        
        // Update board information with better null/undefined checking
        document.getElementById('boardInfoName').textContent = board.name || 'Not specified';
        document.getElementById('boardInfoType').textContent = board.type || 'Unknown type';        
        
        // Update connection status with color coding
        const connectionStatusElement = document.getElementById('boardInfoConnectionStatus');
        connectionStatusElement.textContent = board.connected ? 'Connected' : 'Disconnected';
        connectionStatusElement.className = board.connected ? 'status-connected' : 'status-disconnected';
        
        document.getElementById('boardInfoSlaveId').textContent = board.slave_id || 'N/A';
        document.getElementById('boardInfoModbusPort').textContent = board.modbus_port !== undefined ? `Port ${board.modbus_port + 1}` : 'N/A';
        document.getElementById('boardInfoPollTime').textContent = board.poll_time ? `${board.poll_time} ms` : 'N/A';
        
        // Grey out board info if disconnected
        const boardInfoContainer = document.getElementById('boardInfo');
        if (boardInfoContainer) {
            boardInfoContainer.className = board.connected ? 'board-info-connected' : 'board-info-disconnected';
        }

        // Update connection and initialisation status
        connectStatusGlobal[boardId] = board.connected;
        initStatusGlobal[boardId] = board.initialised;
        
        // Handle board type specific content
        if (board.type === 'Thermocouple IO') {
            document.getElementById('thermocoupleContent').style.display = 'block';
            document.getElementById('thermocoupleOverview').style.display = 'block';
            
            // Check if thermocouple data exists
            if (!board.thermocouple) {
                console.error('Thermocouple data missing from API response');
                showToast('error', 'Error', 'Failed to load thermocouple data');
                return;
            }
            
            // Update error indicators (check if errors object exists)
            if (board.thermocouple.errors) {
                updateErrorIndicator('modbusError', board.thermocouple.errors.modbus);
                updateErrorIndicator('i2cError', board.thermocouple.errors.i2c);
                updateErrorIndicator('psuError', board.thermocouple.errors.psu);
            } else {
                console.warn('Error indicators missing in API response');
            }
            
            // Update PSU voltage
            const psuVoltageElement = document.getElementById('psuVoltage');
            if (psuVoltageElement) {
                const voltageText = (board.thermocouple.psu_voltage !== undefined) 
                    ? `${board.thermocouple.psu_voltage.toFixed(2)}V` 
                    : 'N/A';
                psuVoltageElement.textContent = !board.connected ? `${voltageText} (STALE)` : voltageText;
            }
            
            // Apply offline styling to thermocouple overview panel
            const thermocoupleOverview = document.getElementById('thermocoupleOverview');
            if (thermocoupleOverview) {
                if (!board.connected) {
                    thermocoupleOverview.classList.add('board-offline');
                    thermocoupleOverview.title = 'Board is offline - data may be stale';
                } else {
                    thermocoupleOverview.classList.remove('board-offline');
                    thermocoupleOverview.title = '';
                }
            }
            
            // Check if channels exist before updating
            if (board.thermocouple.channels && Array.isArray(board.thermocouple.channels)) {
                updateThermocoupleChannels(board.thermocouple.channels, board.connected);
                
                // Update temperature history with current data (pass connection status)
                updateClientTemperatureHistory(board.thermocouple.channels, board.connected);
                
                // Update chart with accumulated temperature data
                updateTemperatureChart(clientTemperatureHistory);
            } else {
                console.error('Thermocouple channel data missing or invalid');
                document.getElementById('channelCards').innerHTML = 
                    '<p class="no-boards-message">No thermocouple channel data available</p>';
            }
        } else {
            // Hide thermocouple content for other board types
            document.getElementById('thermocoupleContent').style.display = 'none';
            document.getElementById('thermocoupleOverview').style.display = 'none';
        }
    } catch (error) {
        console.error('Error loading board status:', error);
        showToast('error', 'Error', 'Failed to load board status');
    }
}

// Refresh the current board status
function refreshBoardStatus() {
    if (selectedBoardId && document.getElementById('board-status').classList.contains('active')) {
        loadBoardStatus(selectedBoardId);
    }
}

// Update an error indicator element
function updateErrorIndicator(elementId, hasError) {
    const element = document.getElementById(elementId);
    if (element) {
        element.textContent = hasError ? 'Error' : 'OK';
        element.className = 'error-state ' + (hasError ? 'error' : 'ok');
    }
}

// Update the thermocouple channel cards
function updateThermocoupleChannels(channels, isConnected = true) {
    const channelCardsContainer = document.getElementById('channelCards');
    channelCardsContainer.innerHTML = '';
    
    if (!channels || !Array.isArray(channels) || channels.length === 0) {
        channelCardsContainer.innerHTML = '<p class="no-boards-message">No channel data available</p>';
        return;
    }
    
    // Track if any channel has latched alarms that are active
    let hasActiveAlarms = false;
    let hasLatchedChannels = false;
    
    // Make sure we display all channels (0-7, displayed as 1-8)
    for (let i = 0; i < 8; i++) {
        // Find channel with this number
        const channel = channels.find(ch => ch && ch.number === i);
        
        // If channel doesn't exist, create a placeholder with basic data
        if (!channel) {
            const card = document.createElement('div');
            card.className = 'channel-card';
            
            card.innerHTML = `
                <div class="channel-card-header">
                    <span class="channel-title">Channel ${i + 1}</span>
                    <span class="channel-temp">N/A</span>
                </div>
                <div class="channel-details">
                    <div class="detail-item">
                        <span class="info-label">Type:</span>
                        <span class="info-value">Unknown</span>
                    </div>
                    <div class="detail-item">
                        <span class="info-label">Setpoint:</span>
                        <span class="info-value">N/A</span>
                    </div>
                    <div class="detail-item">
                        <span class="info-label">Hysteresis:</span>
                        <span class="info-value">N/A</span>
                    </div>
                    <div class="detail-item">
                        <span class="info-label">Cold Junction:</span>
                        <span class="info-value">N/A</span>
                    </div>
                </div>
            `;
            
            channelCardsContainer.appendChild(card);
            continue;
        }
        
        const card = document.createElement('div');
        card.className = 'channel-card';
        
        // Apply offline styling if board is disconnected
        if (!isConnected) {
            card.classList.add('board-offline');
            card.title = 'Board is offline - data may be stale';
        }
        
        // Determine temperature display class based on status
        let tempClass = '';
        if (channel.status && channel.status.alarm_state) {
            tempClass = 'alarm';
        } else if ((channel.status && channel.status.open_circuit) || 
                  (channel.status && channel.status.short_circuit)) {
            tempClass = 'fault';
        }
        
        // Format temperature value
        let tempDisplay = 'N/A';
        if (typeof channel.temperature === 'number') {
            if (channel.status && (channel.status.open_circuit || channel.status.short_circuit)) {
                tempDisplay = 'Open Circuit';
            } else {
                tempDisplay = channel.temperature.toFixed(1) + '°C';
                // Add offline indicator if board is disconnected
                if (!isConnected) {
                    tempDisplay += ' (OFFLINE)';
                }
            }
        } else if (!isConnected) {
            tempDisplay = 'N/A (OFFLINE)';
        }
        
        // Lookup TC type text (tc_type 0-7 maps to K, J, T, etc.)
        const tcTypeText = thermocoupleLookup[channel.tc_type] || 'Unknown';
        
        // Safely format numbers
        const formatValue = (value, decimals = 1) => {
            if (typeof value === 'number') {
                return value.toFixed(decimals);
            }
            return 'N/A';
        };
        
        // Check if channel has latch enabled and if alarm is active
        const isLatchEnabled = channel.settings && channel.settings.alert_latch;
        const isAlarmActive = channel.status && channel.status.alarm_state;
        
        // Check if temperature is below setpoint (only enable reset if temp is safe)
        const tempValue = typeof channel.temperature === 'number' ? channel.temperature : null;
        const setpointValue = typeof channel.alert_setpoint === 'number' ? channel.alert_setpoint : null;
        const isBelowSetpoint = tempValue !== null && setpointValue !== null && tempValue < setpointValue;
        
        // Update global flags
        if (isLatchEnabled) {
            hasLatchedChannels = true;
            if (isBelowSetpoint) {
                hasActiveAlarms = true;
            }
        }
        
        // Use the channel_name field if available, otherwise use a default name
        const channelName = channel.channel_name || `Channel ${(channel.number !== undefined ? channel.number : i) + 1}`;
        
        // Card HTML structure with possible reset button
        let cardContent = `
            <div class="channel-card-header">
                <span class="channel-title">${channelName}</span>
                <span class="channel-temp ${tempClass}">${tempDisplay}</span>
            </div>
            <div class="channel-details">
                <div class="detail-item">
                    <span class="info-label">Type:</span>
                    <span class="info-value">Type ${tcTypeText}</span>
                </div>
                <div class="detail-item">
                    <span class="info-label">Setpoint:</span>
                    <span class="info-value">${formatValue(channel.alert_setpoint)}°C</span>
                </div>
                <div class="detail-item">
                    <span class="info-label">Hysteresis:</span>
                    <span class="info-value">${channel.alarm_hysteresis !== undefined ? channel.alarm_hysteresis : 'N/A'}°C</span>
                </div>
                <div class="detail-item">
                    <span class="info-label">Cold Junction:</span>
                    <span class="info-value">${formatValue(channel.cold_junction)}°C${!isConnected ? ' (STALE)' : ''}</span>
                </div>
            </div>
            <div class="channel-status-indicators">
                ${channel.settings && channel.settings.alert_enable ? 
                  '<span class="channel-status-indicator enabled">Alert Enabled</span>' : 
                  '<span class="channel-status-indicator disabled">Alert Disabled</span>'}
                ${channel.settings && channel.settings.output_enable ? 
                  '<span class="channel-status-indicator enabled">Output Enabled</span>' : 
                  '<span class="channel-status-indicator disabled">Output Disabled</span>'}
                ${channel.status && channel.status.alarm_state ? 
                  '<span class="channel-status-indicator alarm">Alarm Active</span>' : ''}
                ${channel.status && channel.status.output_state ? 
                  '<span class="channel-status-indicator enabled">Output On</span>' : 
                  '<span class="channel-status-indicator disabled">Output Off</span>'}
                ${(channel.status && (channel.status.open_circuit || channel.status.short_circuit)) ? 
                  '<span class="channel-status-indicator fault">Fault</span>' : ''}
                ${channel.settings && channel.settings.alert_latch ? 
                  '<span class="channel-status-indicator enabled">Latch Enabled</span>' : ''}
            </div>
        `;
        
        // Add reset button for any channel with latching enabled
        if (isLatchEnabled) {
            cardContent += `
            <div class="channel-alarm-actions">
                <button class="btn btn-alarm reset-alarm-btn" data-channel="${channel.number}" ${!isBelowSetpoint ? 'disabled' : ''}>
                    Reset Alarm${!isBelowSetpoint ? ' (Temp > Setpoint)' : ''}
                </button>
            </div>`;
        }
        
        card.innerHTML = cardContent;
        channelCardsContainer.appendChild(card);
    }
    
    // If no cards were added (all channels filtered out), show a message
    if (channelCardsContainer.children.length === 0) {
        channelCardsContainer.innerHTML = '<p class="no-boards-message">No configured thermocouple channels</p>';
    }
    
    // Enable or disable the reset all alarms button
    const resetAllBtn = document.getElementById('resetAllAlarmsBtn');
    if (resetAllBtn) {
        resetAllBtn.disabled = !(hasLatchedChannels && hasActiveAlarms);
    }
    
    // Add event listeners for all reset alarm buttons
    document.querySelectorAll('.reset-alarm-btn').forEach(btn => {
        btn.addEventListener('click', function() {
            const channel = this.getAttribute('data-channel');
            resetChannelAlarm(channel);
        });
    });
}

// Function to reset a specific channel alarm
async function resetChannelAlarm(channel) {
    try {
        if (!selectedBoardId) {
            showToast('error', 'Error', 'No board selected');
            return;
        }
        
        // Ensure channel is just a number (remove any extra characters)
        const channelNum = parseInt(channel);
        
        const response = await fetch(`/api/status/reset_alarm?id=${selectedBoardId}&channel=${channelNum}`);
        
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const data = await response.json();
        
        if (data.success) {
            showToast('success', 'Success', `Reset alarm for channel ${channelNum + 1}`);
            // Refresh the board status to update UI
            loadBoardStatus(selectedBoardId);
        } else {
            throw new Error(data.error || 'Failed to reset alarm');
        }
    } catch (error) {
        console.error('Error resetting channel alarm:', error);
        showToast('error', 'Error', `Failed to reset alarm: ${error.message}`);
    }
}

// Function to reset all alarms
async function resetAllAlarms() {
    try {
        if (!selectedBoardId) {
            showToast('error', 'Error', 'No board selected');
            return;
        }
        
        const response = await fetch(`/api/status/reset_all_alarms?id=${selectedBoardId}`);
        
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const data = await response.json();
        
        if (data.success) {
            showToast('success', 'Success', 'Reset all alarms');
            // Refresh the board status to update UI
            loadBoardStatus(selectedBoardId);
        } else {
            throw new Error(data.error || 'Failed to reset alarms');
        }
    } catch (error) {
        console.error('Error resetting all alarms:', error);
        showToast('error', 'Error', `Failed to reset alarms: ${error.message}`);
    }
}

// Initialize the board status page
function initBoardStatusPage() {
    console.log('Initializing board status page');
    
    // Clear any existing interval
    if (statusRefreshInterval) {
        clearInterval(statusRefreshInterval);
    }
    
    // Load the board list
    loadBoardStatusList();
    
    // Set up the board selector change event
    const boardSelector = document.getElementById('statusBoardSelect');
    if (boardSelector) {
        boardSelector.addEventListener('change', function() {
            selectedBoardId = this.value;
            if (selectedBoardId) {
                loadBoardStatus(selectedBoardId);
                
                // Reset the temperature history and chart when switching boards
                resetClientTemperatureHistory();
                if (boardStatusChart) {
                    boardStatusChart.destroy();
                    boardStatusChart = null;
                }
            }
        });
    }
    
    // Set up the reset all alarms button
    const resetAllBtn = document.getElementById('resetAllAlarmsBtn');
    if (resetAllBtn) {
        resetAllBtn.addEventListener('click', resetAllAlarms);
    }
    
    // Start the refresh interval (every 2 seconds)
    statusRefreshInterval = setInterval(refreshBoardStatus, 2000);

}

// Update client-side temperature history
function updateClientTemperatureHistory(channels, isConnected = true) {
    // Add current timestamp (seconds since start)
    const currentTime = new Date().getTime() / 1000;
        
    // Initialize the channels array if it's empty
    if (clientTemperatureHistory.channels.length === 0) {
        // Create 8 channel objects with empty data arrays
        for (let i = 0; i < 8; i++) {
            clientTemperatureHistory.channels[i] = {
                number: i,
                tc_type: channels.find(ch => ch && ch.number === i)?.tc_type || 0,
                data: []
            };
        }
    }
    
    // Add current timestamp
    clientTemperatureHistory.timestamps.push(currentTime);
    
    // Limit the number of timestamps
    if (clientTemperatureHistory.timestamps.length > CLIENT_HISTORY_MAX_POINTS) {
        clientTemperatureHistory.timestamps.shift();
    }
    
    // Update each channel's data
    for (let i = 0; i < 8; i++) {
        // Find channel with this number
        const channel = channels.find(ch => ch && ch.number === i);
        
        // Add the temperature data (or null if not available or board offline)
        let tempValue = null;
        if (isConnected && channel && typeof channel.temperature === 'number') {
            if (channel.status && (channel.status.open_circuit || channel.status.short_circuit)) {
                tempValue = null; // Use null for open/short circuit
            } else {
                tempValue = channel.temperature;
            }
        }
        // If board is offline, tempValue remains null to create gaps in chart
        
        // Add the data point
        clientTemperatureHistory.channels[i].data.push(tempValue);
        
        // Limit the number of data points
        if (clientTemperatureHistory.channels[i].data.length > CLIENT_HISTORY_MAX_POINTS) {
            clientTemperatureHistory.channels[i].data.shift();
        }
    }
}

// Update the temperature chart with historical data
function updateTemperatureChart(data) {
    const ctx = document.getElementById('temperatureChart');
    if (!ctx) return;
    
    // If the chart already exists, update it instead of recreating
    if (boardStatusChart) {
        // Update the labels
        boardStatusChart.data.labels = data.timestamps.map(timestamp => {
            const currentTime = new Date().getTime() / 1000;
            const secondsAgo = Math.round(currentTime - timestamp);
            return secondsAgo >= 0 ? secondsAgo : 0;
        }).map(seconds => {
            if (seconds < 60) {
                return seconds + 's';
        } else {
                const minutes = Math.floor(seconds / 60);
                const remainingSeconds = seconds % 60;
                return `${minutes}m${remainingSeconds > 0 ? remainingSeconds + 's' : ''}`;
        }
        });
        
        // Update or add datasets
        data.channels.forEach((newDataset, i) => {
            // If we already have this dataset, update it
            if (i < boardStatusChart.data.datasets.length) {
                boardStatusChart.data.datasets[i].data = newDataset.data;
                // Preserve visibility state
                // No need to update other properties (color, etc.)
            } else {
                // Add new dataset if it doesn't exist
                boardStatusChart.data.datasets.push({
                    label: `Ch. ${newDataset.number + 1}`,
                    data: newDataset.data,
                    borderColor: chartColors[i % chartColors.length],
                    backgroundColor: chartColors[i % chartColors.length] + '20', // Add transparency for fill
                    fill: false,
                    tension: 0.2,
                    pointRadius: 1,
                    borderWidth: 2
                });
    }
        });
        
        // Remove extra datasets if there are fewer now
        if (boardStatusChart.data.datasets.length > data.channels.length) {
            boardStatusChart.data.datasets.length = data.channels.length;
        }
        
        // Update the chart
        boardStatusChart.update();
        
        // Update legend if needed
        updateChartLegend(data.channels);
    } else {
        // Create legend items (only the first time)
        const legendContainer = document.getElementById('chartLegend');
        if (legendContainer) {
            legendContainer.innerHTML = '';
            
            // Create legend items
            data.channels.forEach((channel, index) => {
                if (!channel || !channel.data || !Array.isArray(channel.data)) return;
                if (channel.data.length === 0) return;
                
                const color = chartColors[index % chartColors.length];
                
                // Create legend item
                const legendItem = document.createElement('div');
                legendItem.className = 'legend-item';
                legendItem.dataset.index = index; // Store the dataset index
                
                // Get channel number and type
                const channelNumber = channel.number + 1; // Convert 0-based to 1-based for display
                const tcTypeName = thermocoupleLookup[channel.tc_type] || 'Unknown';
                
                legendItem.innerHTML = `
                    <div class="legend-color" style="background-color: ${color};"></div>
                    <div class="legend-label">Channel ${channelNumber} (Type ${tcTypeName})</div>
                `;
                
                // Add click event to toggle visibility
                legendItem.addEventListener('click', function() {
                    const index = parseInt(this.dataset.index);
                    if (boardStatusChart && typeof boardStatusChart.isDatasetVisible === 'function') {
                        const visibility = boardStatusChart.isDatasetVisible(index);
                        boardStatusChart.setDatasetVisibility(index, !visibility);
                        this.classList.toggle('disabled', visibility);
                        boardStatusChart.update();
                    }
                });
                
                legendContainer.appendChild(legendItem);
            });
        }
        
        // Create the chart for the first time
        boardStatusChart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: data.timestamps.map(timestamp => {
                    const currentTime = new Date().getTime() / 1000;
                    const secondsAgo = Math.round(currentTime - timestamp);
                    return secondsAgo >= 0 ? secondsAgo : 0;
                }).map(seconds => {
                    if (seconds < 60) {
                        return seconds + 's';
        } else {
                        const minutes = Math.floor(seconds / 60);
                        const remainingSeconds = seconds % 60;
                        return `${minutes}m${remainingSeconds > 0 ? remainingSeconds + 's' : ''}`;
        }
                }),
                datasets: data.channels.map((channel, index) => ({
                    label: `Ch. ${channel.number + 1}`,
                    data: channel.data,
                    borderColor: chartColors[index % chartColors.length],
                    backgroundColor: chartColors[index % chartColors.length] + '20', // Add transparency for fill
                    fill: false,
                    tension: 0.2,
                    pointRadius: 1,
                    borderWidth: 2
                }))
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                animation: {
                    duration: 500 // Shorter animation for smoother updates
                },
                plugins: {
                    legend: {
                        display: false // We use our custom legend
                    },
                    tooltip: {
                        mode: 'index',
                        intersect: false,
                        callbacks: {
                            label: function(context) {
                                const label = context.dataset.label || '';
                                const value = context.parsed.y;
                                return `${label}: ${value !== null ? value.toFixed(1) : 'N/A'}°C`;
                            },
                            title: function(tooltipItems) {
                                if (tooltipItems && tooltipItems.length > 0) {
                                    return `${tooltipItems[0].label} ago`;
                                }
                                return '';
                            }
                        }
                    }
                },
                scales: {
                    x: {
                        title: {
                            display: true,
                            text: 'Time ago'
                        },
                        reverse: true, // This makes newest data (lowest seconds ago) appear on the right
                        ticks: {
                            maxTicksLimit: 10
                        }
                    },
                    y: {
                        title: {
                            display: true,
                            text: 'Temperature (°C)'
                        }
                    }
                }
            }
        });
    }
}

// Function to update the chart legend without recreating it
function updateChartLegend(channels) {
    const legendContainer = document.getElementById('chartLegend');
    if (!legendContainer) return;
    
    // Update existing legend items without recreating them
    const legendItems = legendContainer.querySelectorAll('.legend-item');
    
    channels.forEach((channel, index) => {
        if (!channel || !channel.data || !Array.isArray(channel.data)) return;
        if (channel.data.length === 0) return;
        
        // Update existing legend item if it exists
        if (index < legendItems.length) {
            const tcTypeName = thermocoupleLookup[channel.tc_type] || 'Unknown';
            const channelNumber = channel.number + 1;
            
            // Only update the text content, not the entire innerHTML to preserve event listeners
            const labelElement = legendItems[index].querySelector('.legend-label');
            if (labelElement) {
                labelElement.textContent = `Channel ${channelNumber} (Type ${tcTypeName})`;
            }
            }
        });
    }
    
// Initialize board status when switching to that tab
document.addEventListener('DOMContentLoaded', function() {
    const boardStatusTab = document.querySelector('a[data-page="board-status"]');
    if (boardStatusTab) {
        boardStatusTab.addEventListener('click', function() {
            // Initialize the board status page when switching to it
            initBoardStatusPage();
        });
    }
    
    // Add Chart.js library dynamically with CDN primary and local fallback
    if (!window.Chart) {
        console.log('Loading Chart.js...');
        
        // Create a function to load the local version
        function loadLocalChart() {
            console.log('Attempting to load local Chart.js');
            const localScript = document.createElement('script');
            localScript.src = 'script/chart.js';
            localScript.onload = function() {
                console.log('Local Chart.js loaded successfully');
            };
            localScript.onerror = function() {
                console.error('Failed to load Chart.js from local fallback');
            };
            document.head.appendChild(localScript);
        }
        
        // Try CDN first with a timeout
        const script = document.createElement('script');
        script.src = 'https://cdn.jsdelivr.net/npm/chart.js';
        
        // Set a timeout to try local version if CDN is taking too long (likely offline)
        const timeoutId = setTimeout(function() {
            console.log('CDN load timeout - falling back to local version');
            loadLocalChart();
        }, 3000); // 3 second timeout
        
        script.onload = function() {
            clearTimeout(timeoutId); // Cancel the timeout
            console.log('Chart.js loaded successfully from CDN');
        };
        
        script.onerror = function() {
            clearTimeout(timeoutId); // Cancel the timeout
            console.log('Failed to load Chart.js from CDN, falling back to local version');
            loadLocalChart();
        };
        
        document.head.appendChild(script);
    }
});

// Dashboard functionality
let dashboardItems = [];
let dashboardEditMode = false;
let dashboardRefreshInterval = null;

// Initialize dashboard
function initDashboard() {
    console.log('Initializing dashboard');
    
    // Set up event listeners for dashboard controls
    const editBtn = document.getElementById('editDashboardBtn');
    const saveBtn = document.getElementById('saveDashboardBtn');
    const cancelBtn = document.getElementById('cancelDashboardBtn');
    
    if (editBtn) {
        editBtn.addEventListener('click', enableDashboardEditMode);
    }
    
    if (saveBtn) {
        saveBtn.addEventListener('click', saveDashboardOrder);
    }
    
    if (cancelBtn) {
        cancelBtn.addEventListener('click', disableDashboardEditMode);
    }
    
    // Load dashboard items
    loadDashboardItems();
}

// Load dashboard items from the server
async function loadDashboardItems() {
    try {
        const response = await fetch('/api/dashboard/items');
        const data = await response.json();
        
        dashboardItems = data.items || [];
        
        // Sort items by display order
        dashboardItems.sort((a, b) => a.display_order - b.display_order);
        
        // Render dashboard items
        renderDashboardItems();
        
        // Start refresh interval if not already running
        if (!dashboardRefreshInterval) {
            dashboardRefreshInterval = setInterval(refreshDashboardData, 2000);
        }
        
    } catch (error) {
        console.error('Error loading dashboard items:', error);
        showToast('error', 'Error', 'Failed to load dashboard items');
    }
}

// Render dashboard items
function renderDashboardItems() {
    const container = document.getElementById('dashboardItemsContainer');
    const noItemsMessage = document.getElementById('noDashboardItems');
    
    if (!container) return;
    
    // Clear container
    container.innerHTML = '';
    
    // Show/hide no items message
    if (dashboardItems.length === 0) {
        if (noItemsMessage) {
            noItemsMessage.style.display = 'block';
        }
        container.style.display = 'none';
        return;
    } else {
        if (noItemsMessage) {
            noItemsMessage.style.display = 'none';
        }
        container.style.display = 'grid';
    }
    
    // Create items
    dashboardItems.forEach(item => {
        const dashboardItem = document.createElement('div');
        dashboardItem.className = 'dashboard-item';
        dashboardItem.id = `dashboard-item-${item.board_index}-${item.channel_index}`;
        dashboardItem.dataset.boardIndex = item.board_index;
        dashboardItem.dataset.channelIndex = item.channel_index;
        dashboardItem.dataset.displayOrder = item.display_order;
        
        const boardName = document.createElement('div');
        boardName.className = 'board-name';
        boardName.textContent = item.board_name;
        
        const channelName = document.createElement('div');
        channelName.className = 'channel-name';
        channelName.textContent = item.channel_name;
        
        const temperature = document.createElement('div');
        temperature.className = 'temperature';
        temperature.innerHTML = '&ndash;.&ndash; °C';
        
        const coldJunction = document.createElement('div');
        coldJunction.className = 'cold-junction';
        coldJunction.innerHTML = 'CJ: &ndash;.&ndash; °C';
        
        const outputStatus = document.createElement('div');
        outputStatus.className = 'output-status';
        outputStatus.innerHTML = '<span class="output-indicator"></span> Output: &ndash;';
        
        const alertStatus = document.createElement('div');
        alertStatus.className = 'alert-status';
        
        // Add elements to item
        dashboardItem.appendChild(boardName);
        dashboardItem.appendChild(channelName);
        dashboardItem.appendChild(temperature);
        dashboardItem.appendChild(coldJunction);
        dashboardItem.appendChild(outputStatus);
        dashboardItem.appendChild(alertStatus);
        
        // Add item to container
        container.appendChild(dashboardItem);
    });
    
    // Initialize sortable if in edit mode
    if (dashboardEditMode) {
        initSortable();
    }
}

// Refresh dashboard data (temperatures, statuses, etc.)
async function refreshDashboardData() {
    if (dashboardItems.length === 0) return;
    const dashboardTab = document.getElementById('dashboard');
    if (dashboardTab && !dashboardTab.classList.contains('active')) return;
    
    try {
        // Get unique board IDs
        const boardIds = [...new Set(dashboardItems.map(item => item.board_index))];
        
        // Get status for each board
        for (const boardId of boardIds) {
            const response = await fetch(`/api/status/board?id=${boardId}`);
            
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            
            const board = await response.json();
            
            // Check if this is a thermocouple board
            if (board.type === 'Thermocouple IO') {
                // Check if thermocouple data exists
                if (!board.thermocouple || !board.thermocouple.channels) {
                    console.error('Thermocouple data missing from API response');
                    continue;
                }
                
                // Update dashboard with channel data and connection status
                updateDashboardThermocoupleItems(boardId, board.thermocouple.channels, board.connected);
            }
            // Add more board types as needed
        }
    } catch (error) {
        console.error('Error refreshing dashboard data:', error);
    }
}

// Update thermocouple items on the dashboard
function updateDashboardThermocoupleItems(boardId, channels, isConnected = true) {
    const items = dashboardItems.filter(item => item.board_index == boardId);
    
    // Update each item
    items.forEach(item => {
        const channelIndex = item.channel_index;
        if (channelIndex >= channels.length) return;
        
        const channel = channels[channelIndex];
        if (!channel) return;
        
        const dashboardItem = document.getElementById(`dashboard-item-${boardId}-${channelIndex}`);
        if (!dashboardItem) return;
        
        // Apply offline styling if board is disconnected
        if (!isConnected) {
            dashboardItem.classList.add('board-offline');
            dashboardItem.title = 'Board is offline - data may be stale';
        } else {
            dashboardItem.classList.remove('board-offline');
            dashboardItem.title = '';
        }
        
        // Update temperature
        const temperatureElement = dashboardItem.querySelector('.temperature');
        if (temperatureElement) {
            if (!isConnected) {
                // Show stale data with offline indicator
                const staleTemp = channel.temperature !== undefined ? `${channel.temperature.toFixed(1)} °C` : '&ndash;.&ndash; °C';
                temperatureElement.innerHTML = `${staleTemp} <span class="offline-indicator">(OFFLINE)</span>`;
                temperatureElement.style.color = '#999'; // Grey color for offline
            } else if (channel.status && (channel.status.open_circuit || channel.status.short_circuit)) {
                temperatureElement.innerHTML = 'Fault';
                temperatureElement.style.color = 'orange'; // Amber color for faults
            } else {
                temperatureElement.innerHTML = channel.temperature !== undefined ? `${channel.temperature.toFixed(1)} °C` : '&ndash;.&ndash; °C';
                
                // Set color based on alarm state
                if (channel.status && channel.status.alarm_state) {
                    temperatureElement.style.color = 'orangeRed'; // Red for active alerts
                } else {
                    temperatureElement.style.color = '#2196F3'; // Default blue for normal temperatures
                }
            }
        }
        
        // Update cold junction
        const coldJunctionElement = dashboardItem.querySelector('.cold-junction');
        if (coldJunctionElement) {
            const cjTemp = channel.cold_junction !== undefined ? `CJ: ${channel.cold_junction.toFixed(1)} °C` : 'CJ: &ndash;.&ndash; °C';
            if (!isConnected) {
                coldJunctionElement.innerHTML = `${cjTemp} <span class="offline-indicator">(STALE)</span>`;
                coldJunctionElement.style.color = '#999';
            } else {
                coldJunctionElement.innerHTML = cjTemp;
                coldJunctionElement.style.color = '';
            }
        }
        
        // Update output status based on status.output_state (from Board Status API)
        const outputStatusElement = dashboardItem.querySelector('.output-status');
        if (outputStatusElement) {
            // Check if status object exists and contains output_state
            if (channel.status && channel.status.output_state !== undefined) {
                const isOutputOn = channel.status.output_state === true || channel.status.output_state === 1;
                
                if (!isConnected) {
                    // Show stale output status
                    const outputText = isOutputOn ? 'Output: ON' : 'Output: OFF';
                    const indicatorClass = isOutputOn ? 'output-on' : 'output-off';
                    outputStatusElement.innerHTML = `<span class="output-indicator ${indicatorClass}"></span> ${outputText} <span class="offline-indicator">(STALE)</span>`;
                    outputStatusElement.style.color = '#999';
                } else if (isOutputOn) {
                    outputStatusElement.innerHTML = '<span class="output-indicator output-on"></span> Output: ON';
                    outputStatusElement.style.color = '';
                } else {
                    outputStatusElement.innerHTML = '<span class="output-indicator output-off"></span> Output: OFF';
                    outputStatusElement.style.color = '';
                }
            } else {
                outputStatusElement.innerHTML = '<span class="output-indicator"></span> Output: &ndash;';
                outputStatusElement.style.color = !isConnected ? '#999' : '';
            }
        }
        
        // Update alert status based on status.alarm_state (from Board Status API)
        const alertStatusElement = dashboardItem.querySelector('.alert-status');
        if (alertStatusElement) {
            alertStatusElement.className = 'alert-status';
            
            // Check if status object exists and contains alarm_state
            if (channel.status && channel.status.alarm_state) {
                alertStatusElement.classList.add('alert-active');
                if (!isConnected) {
                    alertStatusElement.classList.add('stale-data');
                }
            }
            
            if (!isConnected) {
                alertStatusElement.classList.add('board-offline');
            }
        }
    });
}

// Enable dashboard edit mode
function enableDashboardEditMode() {
    dashboardEditMode = true;
    
    // Show/hide buttons
    document.getElementById('editDashboardBtn').style.display = 'none';
    document.getElementById('saveDashboardBtn').style.display = 'inline-block';
    document.getElementById('cancelDashboardBtn').style.display = 'inline-block';
    
    // Add edit mode class to container
    const container = document.getElementById('dashboardItemsContainer');
    if (container) {
        container.classList.add('dashboard-edit-mode');
    }
    
    // Initialize sortable
    initSortable();
    
    // Show toast
    showToast('info', 'Edit Mode', 'Dashboard is now in edit mode. Drag and drop items to rearrange them.');
}

// Disable dashboard edit mode
function disableDashboardEditMode() {
    dashboardEditMode = false;
    
    // Show/hide buttons
    document.getElementById('editDashboardBtn').style.display = 'inline-block';
    document.getElementById('saveDashboardBtn').style.display = 'none';
    document.getElementById('cancelDashboardBtn').style.display = 'none';
    
    // Remove edit mode class from container
    const container = document.getElementById('dashboardItemsContainer');
    if (container) {
        container.classList.remove('dashboard-edit-mode');
    }
    
    // Reinitialize items (discard changes)
    loadDashboardItems();
}

// Initialize sortable for drag & drop reordering
function initSortable() {
    const container = document.getElementById('dashboardItemsContainer');
    if (!container) return;
    
    // Check if Sortable library is available
    if (typeof Sortable !== 'undefined') {
        // Create new sortable instance
        new Sortable(container, {
            animation: 150,
            ghostClass: 'dashboard-item-placeholder',
            onEnd: function(evt) {
                // Update display order of all items
                const items = container.querySelectorAll('.dashboard-item');
                items.forEach((item, index) => {
                    item.dataset.displayOrder = index;
                });
            }
        });
    } else {
        console.error('Sortable library not loaded. Please include Sortable.js in your project.');
        showToast('error', 'Error', 'Sortable library not loaded. Edit mode may not work correctly.');
        
        // Load Sortable.js dynamically
        const script = document.createElement('script');
        script.src = 'https://cdn.jsdelivr.net/npm/sortablejs@latest/Sortable.min.js';
        script.onload = function() {
            initSortable();
        };
        document.head.appendChild(script);
    }
}

// Save dashboard order
async function saveDashboardOrder() {
    try {
        // Get items from container
        const container = document.getElementById('dashboardItemsContainer');
        const items = container.querySelectorAll('.dashboard-item');
        
        // Create items array
        const updatedItems = [];
        items.forEach((item, index) => {
            updatedItems.push({
                board_index: parseInt(item.dataset.boardIndex),
                channel_index: parseInt(item.dataset.channelIndex),
                display_order: index
            });
        });
        
        // Update dashboardItems array
        dashboardItems = updatedItems.map(item => ({
            ...dashboardItems.find(
                existingItem => existingItem.board_index == item.board_index && 
                               existingItem.channel_index == item.channel_index
            ),
            display_order: item.display_order
        }));
        
        // Save to server
        const response = await fetch('/api/dashboard/order', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ items: updatedItems })
        });
        
        const data = await response.json();
        
        if (data.status === 'success') {
            showToast('success', 'Success', 'Dashboard layout saved successfully');
            disableDashboardEditMode();
        } else {
            throw new Error(data.message || 'Failed to save dashboard layout');
        }
    } catch (error) {
        console.error('Error saving dashboard layout:', error);
        showToast('error', 'Error', 'Failed to save dashboard layout');
    }
}

// Initialize dashboard when switching to that tab
document.addEventListener('DOMContentLoaded', function() {
    const dashboardTab = document.querySelector('a[data-page="dashboard"]');
    if (dashboardTab) {
        dashboardTab.addEventListener('click', function() {
            // Check if dashboard needs reloading due to board config changes
            const needsReload = window.dashboardNeedsReload || false;
            
            // Reset the flag
            window.dashboardNeedsReload = false;
            
            // Clear any existing refresh interval
            if (dashboardRefreshInterval) {
                clearInterval(dashboardRefreshInterval);
                dashboardRefreshInterval = null;
            }
            
            // Initialize the dashboard (this will load items and start the refresh interval)
            initDashboard();
            
            // If needed, show a toast notification that the dashboard is being refreshed
            if (needsReload) {
                showToast('info', 'Dashboard Updated', 'Refreshing dashboard with latest configuration changes');
            }
        });
    }
    
    // Initialize dashboard on page load if dashboard tab is active
    if (document.querySelector('#dashboard').classList.contains('active')) {
        initDashboard();
    }
});

// Page loading indicator
function hideLoadingOverlay() {
    const overlay = document.getElementById('loadingOverlay');
    if (overlay) {
        overlay.style.opacity = '0';
        setTimeout(() => {
            overlay.style.display = 'none';
        }, 300);
    }
}

// Show the loading overlay when navigating away from the page
window.addEventListener('beforeunload', function() {
    const overlay = document.getElementById('loadingOverlay');
    if (overlay) {
        overlay.style.opacity = '1';
        overlay.style.display = 'flex';
    }
});

// Hide loading overlay when page is fully loaded
window.addEventListener('load', function() {
    // Wait a short time to ensure all resources are loaded
    setTimeout(hideLoadingOverlay, 500);
});

// Also hide loading overlay if it's taking too long (failsafe)
setTimeout(hideLoadingOverlay, 15000);

// Function to export board configuration
function exportBoardConfiguration() {
    console.log('Exporting board configuration');
    // Show loading toast
    const loadingToast = showToast('info', 'Exporting...', 'Preparing configuration file for download', 5000);
    
    // Create a download link and trigger it
    const downloadLink = document.createElement('a');
    downloadLink.href = '/api/boards/export';
    downloadLink.download = 'board_config.json';
    
    // Add event listener to remove loading toast when download starts
    downloadLink.addEventListener('click', () => {
        if (loadingToast.parentNode) {
            loadingToast.parentNode.removeChild(loadingToast);
        }
        showToast('success', 'Success', 'Board configuration exported successfully');
    });
    
    // Append, click, and remove
    document.body.appendChild(downloadLink);
    downloadLink.click();
    document.body.removeChild(downloadLink);
}

// Function to import board configuration
function importBoardConfiguration(file) {
    if (!file) {
        showToast('error', 'Error', 'No file selected for import');
        return;
    }
    
    console.log('Importing board configuration from file:', file.name);
    
    // Check file type
    if (!file.name.toLowerCase().endsWith('.json')) {
        showToast('error', 'Error', 'Selected file is not a JSON file');
        return;
    }
    
    // Store the file temporarily to be used after confirmation
    window.selectedConfigFile = file;
    
    // Show the confirmation modal
    const importConfirmModal = document.getElementById('importConfirmModal');
    importConfirmModal.style.display = 'block';
}

// Function to actually perform the import after confirmation
function performImport(file, importMode = 'overwrite') {
    // Show loading toast with appropriate message
    const loadingMessage = importMode === 'online' 
        ? 'Importing configuration and bringing boards online...' 
        : 'Uploading and validating configuration file...';
    const loadingToast = showToast('info', 'Importing...', loadingMessage, 10000);
    
    const xhr = new XMLHttpRequest();
    const formData = new FormData();
    formData.append('file', file);
    // Note: import_mode is now sent as URL parameter instead of form data
    
    xhr.onload = function() {
        // Remove loading toast
        if (loadingToast.parentNode) {
            loadingToast.parentNode.removeChild(loadingToast);
        }
        
        if (xhr.status === 200) {
            console.log('Import successful:', xhr.responseText);
            
            // Show appropriate success message based on import mode
            const successMessage = importMode === 'online' 
                ? 'Board configuration imported and boards are now online!' 
                : 'Board configuration imported successfully';
            showToast('success', 'Success', successMessage);
            
            // Refresh the board configurations list after a short delay to allow
            // the server to fully process the configuration
            setTimeout(() => {
                loadBoardConfigurations();
            }, 500);
        } else {
            console.error('Error importing configuration:', xhr.responseText);
            showToast('error', 'Import Failed', xhr.responseText || 'Failed to import configuration');
        }
    };
    
    xhr.onerror = function() {
        // Remove loading toast
        if (loadingToast.parentNode) {
            loadingToast.parentNode.removeChild(loadingToast);
        }
        
        console.error('Network error during import');
        showToast('error', 'Import Failed', 'Network error during file upload');
    };
    
    // Open and send the request with import mode as URL parameter
    xhr.open('POST', '/api/boards/import?import_mode=' + encodeURIComponent(importMode), true);
    xhr.send(formData);
    
    // Reset the file input
    document.getElementById('configFileInput').value = '';
}

function resetClientTemperatureHistory() {
    clientTemperatureHistory = {
        timestamps: [],
        channels: []
    };
}