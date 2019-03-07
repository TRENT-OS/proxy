pipeline {
    agent none
    stages {
        stage('checkout') {
            agent any
            steps {
                echo '##################################### Checkout COMPLETED ####################################'
            }
        }
        stage('build') {
            agent {
                docker {
                    image 'camkes_build_env_20190307'
                    // bind the localtime to avoid problems of gaps between the localtime of the container and the host
                    args '-v /etc/localtime:/etc/localtime:ro'
                }
            }
            options { skipDefaultCheckout(true) }
            steps {
                echo '########################################## Building #########################################'
                sh '''#!/bin/bash
                        # install needed dependecies
                        sudo apt-get --assume-yes install libvdeplug2-dev vde2

                        ./build.sh
                    '''
                cleanWs()
            }
        }
    }
}
