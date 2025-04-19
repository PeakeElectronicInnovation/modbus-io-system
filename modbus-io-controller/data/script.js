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

            // Set NTP server
            document.getElementById('hostName').value = data.hostname || '';
            document.getElementById('ntpServer').value = data.ntp || '';
        }
    } catch (error) {
        console.error('Error loading network settings:', error);
    }
}


// Event listeners
document.addEventListener('DOMContentLoaded', () => {
    console.log('DOM loaded - initializing board configuration system');
    
    // Load board configurations on page load
    loadBoardConfigurations();
    
    // Set up an auto-refresh for board configurations every 5 seconds for testing
    console.log("Setting up auto-refresh for board configurations");
    setInterval(loadBoardConfigurations, 5000);
    
    loadInitialSettings();  // Load initial NTP and timezone settings
    loadNetworkSettings();  // Load initial network settings
    updateLiveClock();
    updateNetworkInfo();    // Update network status info
    
    // Initialize system status if system tab is active initially
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
setInterval(updateLiveClock, 1000);    // Update clock every second
setInterval(updateNetworkInfo, 5000);  // Update network info every 5 seconds

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
              
        // Update Modbus status
        const modbusStatus = document.getElementById('modbusStatus');
        modbusStatus.textContent = data.modbus ? 'CONNECTED' : 'NOT-CONNECTED';
        modbusStatus.className = 'status ' + (data.modbus ? 'connected' : 'not-connected');
        
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
                document.getElementById('sdLogSizeContainer').style.display = 'flex';
                document.getElementById('sdSensorSizeContainer').style.display = 'flex';
                
                // Update SD card details
                document.getElementById('sdCapacity').textContent = data.sd.capacityGB.toFixed(1) + ' GB';
                document.getElementById('sdFreeSpace').textContent = data.sd.freeSpaceGB.toFixed(1) + ' GB';
                document.getElementById('sdLogSize').textContent = data.sd.logFileSizeKB.toFixed(1) + ' kB';
                document.getElementById('sdSensorSize').textContent = data.sd.sensorFileSizeKB.toFixed(1) + ' kB';
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
    document.getElementById('sdLogSizeContainer').style.display = 'none';
    document.getElementById('sdSensorSizeContainer').style.display = 'none';
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
    
    // Create toast element
    const toast = document.createElement('div');
    toast.className = `toast toast-${type}`;
    
    // Add icon based on type
    let iconClass = '';
    switch (type) {
        case 'success':
            iconClass = '✓';
            break;
        case 'error':
            iconClass = '✗';
            break;
        case 'info':
            iconClass = 'ℹ';
            break;
        default:
            iconClass = 'ℹ';
    }
    
    // Create toast content
    toast.innerHTML = `
        <div class="toast-icon">${iconClass}</div>
        <div class="toast-content">
            <div class="toast-title">${title}</div>
            <div class="toast-message">${message}</div>
        </div>
    `;
    
    // Add to container
    toastContainer.appendChild(toast);
    
    // Remove after duration
    setTimeout(() => {
        toast.classList.add('toast-exit');
        setTimeout(() => {
            toastContainer.removeChild(toast);
        }, 300); // Wait for exit animation to complete
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
        dst: document.getElementById('enableDST').checked
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
                    throw new Error('Failed to reboot system');
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
    
    // Add a short delay before first API call after initialization
    // This helps ensure the SD card is fully ready
    const initialDelay = (retryCount === 0) ? 500 : 0;
    
    setTimeout(() => {
        fetch(`/api/sd/list?path=${encodeURIComponent(path)}`)
            .then(response => {
                if (!response.ok) {
                    // If we get a non-OK response but the SD is inserted according to the system status
                    // it could be a timing issue with the SD card initialization
                    if (response.status === 503 && retryCount < maxRetries) {
                        // Wait longer with each retry
                        const delay = 1000 * (retryCount + 1);
                        console.log(`SD card not ready, retrying in ${delay}ms (attempt ${retryCount + 1}/${maxRetries})`);
                        fileListContainer.innerHTML = `<div class="loading">SD card initializing, please wait... (${retryCount + 1}/${maxRetries})</div>`;
                        
                        setTimeout(() => {
                            loadDirectory(path, retryCount + 1);
                        }, delay);
                        return null; // Skip further processing for this attempt
                    }
                    
                    throw new Error(response.status === 503 ? 
                        'SD card not ready. It may be initializing or experiencing an issue.' :
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
                sdStatusElement.innerHTML = '<div class="status-info">SD Card initializing, please wait...</div>';
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

// Initialize the file manager
function initFileManager() {
    if (!document.getElementById('filemanager')) return;
    
    console.log('Initializing file manager');
    fileManagerActive = true;
    
    // Initialize file manager
    checkSDCardStatus();
    
    // Start periodic SD card status checks
    if (sdStatusInterval) {
        clearInterval(sdStatusInterval);
    }
    sdStatusInterval = setInterval(checkSDCardStatus, 3000); // Check every 3 seconds
    
    // Initialize with our improved function
    checkAndLoadDirectory();
}

// Function to navigate to a directory
function navigateTo(path) {
    if (fileManagerActive) {
        loadDirectory(path);
    }
}

// Initialize File Manager when switching to the tab
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
    
    // Initialize if file manager is the active tab on page load
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

// Function to convert board type string to enum value
function getBoardTypeValue(boardType) {
    switch (boardType) {
        case 'THERMOCOUPLE_IO':
            return 0;
        case 'UNIVERSAL_IN':
            return 1;
        case 'DIGITAL_IO':
            return 2;
        case 'POWER_METER':
            return 3;
        default:
            return 0; // Default to thermocouple
    }
}

// Load board configurations from backend
async function loadBoardConfigurations() {
    console.log("ATTEMPTING to fetch board configurations...");
    
    // Use only the known-working endpoint and add retry logic
    const maxRetries = 3;
    let retryCount = 0;
    let lastError = null;
    
    while (retryCount < maxRetries) {
        try {
            // Add a cache-busting parameter to avoid browser caching
            const timestamp = new Date().getTime();
            const endpoint = `/api/boards/all?t=${timestamp}`;
            
            console.log(`Attempt ${retryCount + 1}/${maxRetries} - Trying endpoint: ${endpoint}`);
            
            const response = await fetch(endpoint, {
                method: 'GET',
                headers: {
                    'Cache-Control': 'no-cache',
                    'Pragma': 'no-cache'
                }
            });
            
            // If we got a successful response, process it
            if (response.ok) {
                console.log(`Successful response from ${endpoint}`);
                const responseText = await response.text();
                console.log("Raw board API response:", responseText);
                
                // Try to parse the response
                let data;
                try {
                    data = JSON.parse(responseText);
                } catch (parseError) {
                    console.error("Failed to parse JSON response:", parseError);
                    throw parseError;
                }
                
                console.log("Parsed board configurations:", data);
                
                if (data && data.boards) {
                    boardConfigurations = data.boards;
                    console.log("Board configurations set to:", boardConfigurations);
                    if (boardConfigurations.length > 0) {
                        console.log("First board:", boardConfigurations[0]);
                    }
                } else {
                    boardConfigurations = [];
                    console.log("No board configurations found in response");
                }
                
                renderBoardsList();
                return; // Success - exit the retry loop
            } else {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
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
    console.error('Error loading board configurations after all retries:', lastError);
    showToast('error', 'Error', 'Failed to load board configurations');
    boardConfigurations = [];
    renderBoardsList();
}

// Add retry logic for the add/update board request
async function sendBoardRequest(url, method, data, successMessage) {
    const maxRetries = 3;
    let retryCount = 0;
    let lastError = null;
    
    while (retryCount < maxRetries) {
        try {
            console.log(`Attempt ${retryCount + 1}/${maxRetries} - Sending ${method} request to ${url}`);
            
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
                console.error("Server response:", errorText);
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            
            showToast('success', 'Success', successMessage);
            return true; // Success
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
    console.error(`Error after ${maxRetries} retries:`, lastError);
    showToast('error', 'Error', 'Failed to save board configuration');
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
            poll_time: parseInt(document.getElementById('pollTime').value)
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
                    alert_hysteresis: alertHysteresis
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
    } catch (error) {
        console.error('Error saving board configuration:', error);
        showToast('error', 'Error', 'Failed to save board configuration');
    }
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
    
    // Determine port number (add 1 for display since ports are 0-indexed)
    let portDisplay = 'Unknown';
    if (board.modbus_port !== undefined) {
        portDisplay = (parseInt(board.modbus_port) + 1).toString();
    }
    
    // Create the board item HTML
    boardItem.innerHTML = `
        <div class="board-info">
            <span class="board-type">${board.name || 'Unnamed Board'}</span>
            <span class="board-details">Type: ${boardTypeName}, Port: ${portDisplay}</span>
        </div>
        <div class="board-actions">
            <button class="btn-edit" data-index="${index}"><i class="fas fa-edit"></i> Edit</button>
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
    
    const deleteBtn = boardItem.querySelector('.btn-delete');
    if (deleteBtn) {
        deleteBtn.addEventListener('click', () => deleteBoard(index));
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
                typeSelectElement.value = 'THERMOCOUPLE_IO';
                break;
            case 1:
                typeSelectElement.value = 'UNIVERSAL_IN';
                break;
            case 2:
                typeSelectElement.value = 'DIGITAL_IO';
                break;
            case 3:
                typeSelectElement.value = 'POWER_METER';
                break;
            default:
                typeSelectElement.value = 'THERMOCOUPLE_IO'; // Default to thermocouple
        }
        
        document.getElementById('boardName').value = board.name || '';
        document.getElementById('modbusPort').value = board.modbus_port;
        document.getElementById('pollTime').value = board.poll_time;
        
        // Show board type specific settings
        showBoardTypeSettings(typeSelectElement.value);
        
        // Fill board type specific settings
        if (board.type === 0) { // THERMOCOUPLE_IO
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
                });
            }
        }
        // Add more board types as needed
    } else {
        // Add new board
        editingBoardIndex = null;
        
        if (formTitle) {
            formTitle.textContent = 'Add New Expansion Board';
        }
        
        // Reset form
        document.getElementById('boardType').value = 'THERMOCOUPLE_IO';
        document.getElementById('boardName').value = '';
        document.getElementById('modbusPort').value = '0';
        document.getElementById('pollTime').value = '1000';
        
        // Show default board type settings
        showBoardTypeSettings('THERMOCOUPLE_IO');
    }
    
    if (boardConfigForm) {
        boardConfigForm.style.display = 'block';
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
                </div>
                <div class="channel-form-right">
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
                        <small class="helper-text">Range: 0-255</small>
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
    console.log('DOM loaded - initializing board configuration system');
    
    // Load board configurations on page load
    loadBoardConfigurations();
    
    // Set up an auto-refresh for board configurations every 5 seconds for testing
    console.log("Setting up auto-refresh for board configurations");
    setInterval(loadBoardConfigurations, 5000);
    
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
});