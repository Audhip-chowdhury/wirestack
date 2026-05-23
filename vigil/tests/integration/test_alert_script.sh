#!/bin/bash
set -e
cd "$(dirname "$0")/../.."
ALERT_SCRIPT="/tmp/vigil-test-alert.sh"
echo '#!/bin/bash' > "$ALERT_SCRIPT"
echo 'echo "$VIGIL_ALERT_TYPE" >> /tmp/vigil-alert-fired.log' >> "$ALERT_SCRIPT"
chmod +x "$ALERT_SCRIPT"
echo "PASS: alert script stub created at $ALERT_SCRIPT"
