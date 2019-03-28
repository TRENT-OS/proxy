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
        stage('prepare_test') {
            agent {
                docker {
                    image 'test_env'
                    args '-v /home/jenkins/.ssh/:/home/jenkins/.ssh:ro -v /etc/localtime:/etc/localtime:ro'
                }
            }
            options { skipDefaultCheckout(true) }
            steps {
                echo '####################################### Update TA ENV #######################################'
                sh  '''#!/bin/bash
                        if [ -d ta ]; then
                            cd ta
                            git pull origin
                            git submodule update --recursive
                        else
                            git clone --recursive -b master ssh://git@bitbucket.hensoldt-cyber.systems:7999/hc/ta.git
                            cd ta
                            python3 -m venv ta-env
                        fi
                        source ta-env/bin/activate
                        pip install -r requirements.txt
                    '''
            }
        }
        stage('test') {
            agent {
                 docker {
                     image 'test_env'
                     args '-v /home/jenkins/.ssh/:/home/jenkins/.ssh:ro -v /etc/localtime:/etc/localtime:ro'
                 }
            }
            options { skipDefaultCheckout(true) }
            steps {
                echo '########################################## Testing ##########################################'
                lock ('AzureCloudConnection') {
                    sh  '''#!/bin/bash
                            workspace=`pwd`
                            source ta/ta-env/bin/activate
                            cd ta/har
                            pytest -v --har_path="${workspace}/ta/mqtt_proxy/files/capdl-loader-image-arm-zynq7000" --proxy_path="${workspace}/build/mqtt_proxy"
                        '''
                }
                cleanWs()
            }
        }


































































    }
}
