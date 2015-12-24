#!/bin/bash

if [ $# -lt 1 ] ; then
	echo "USAGE: ./deploy-setup.sh [key-file]"
	exit 1
fi

if [ ! -f $1 ] ; then
	echo "$1: No such file or directory."
	exit 2
fi

# Setup key constants        
KEY=$1
KEY_PUB=$KEY".pub"

echo ""
echo "Private key file: $KEY"
echo "Public key file:  $KEY_PUB"

read -r -p "Does this look okay (default yes)? (y/n) " prompt

case $prompt in
    [yY][eE][sS]|[yY])
	# Fall through
        ;;
    *)
	echo "Setup failed!"
	exit 3
        ;;
esac

# Inject our key into the authorized keys file
cat $KEY_PUB | nc localhost 8082

# Run some setup commands
scp -P 8081 -i $KEY $KEY_PUB ubuntu@localhost:.ssh/id_rsa.pub
scp -P 8081 -i $KEY $KEY ubuntu@localhost:.ssh/id_rsa
ssh -p 8081 -i $KEY ubuntu@localhost "chmod 700 ~/.ssh"
ssh -p 8081 -i $KEY ubuntu@localhost "chmod 600 ~/.ssh/id_rsa"
ssh -p 8081 -i $KEY ubuntu@localhost "chmod 600 ~/.ssh/id_rsa.pub"
ssh -p 8081 -i $KEY ubuntu@localhost "/home/ubuntu/setup/add-key.sh"

# stop the insecure server
ssh -p 8081 -i $KEY ubuntu@localhost "/home/ubuntu/setup/stop-server.sh" > /dev/null

echo "+---------------------------------------+"
echo "+----- SETUP COMPLETE (SUCCESS) --------+"
echo "+---------------------------------------+"
echo ""
echo "You are now ready to use chronos deploy!!"
echo "Checkout the documentation at: https://wiki.chronos.systems/index.php/Chronos_Deploy"
echo ""
echo "Setup complete."
