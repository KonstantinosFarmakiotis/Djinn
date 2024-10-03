#include "FountainEmitter.h"
#include <iostream>
#include <algorithm>
#include <glfw3.h> // Include GLFW


glm::vec3 function(glm::vec3 position) {
    position.x = sqrt(position.y - 1.0)+0.8;
    position.z = sqrt(position.y - 1.0)+0.3;

    return position;
}

// Η συναρτηση CheckBoxCollision βοηθησε σε μια διαφορετικη υλοποιηση του καπνου αλλα τωρα δεν χρησιμοποιειται
void CheckBoxCollision(glm::vec3 boxcenter, float xoffset, float yoffset, float zoffset, particleAttributes& particle) {

    if (particle.position.y<boxcenter.y + yoffset / 2 && particle.position.y>boxcenter.y - yoffset / 2) {
        if (particle.position.x > boxcenter.x + xoffset / 2 && particle.velocity.x > 0) {
            particle.velocity.x = -particle.velocity.x / (RAND * 0.4 + 1);
            particle.velocity.z = particle.velocity.z * (RAND + 0.5);
        }
        else if (particle.position.x < boxcenter.x - xoffset / 2 && particle.velocity.x < 0) {
            particle.velocity.x = -particle.velocity.x / (RAND * 0.4 + 1);
            particle.velocity.z = particle.velocity.z * (RAND + 0.5);
        }
        if (particle.position.z > boxcenter.z + zoffset / 2 && particle.velocity.z > 0) {
            particle.velocity.z = -particle.velocity.z / (RAND + 1.5);
            particle.velocity.x = particle.velocity.x * (RAND + 0.5);
        }
        else if (particle.position.z < boxcenter.z - zoffset / 2 && particle.velocity.z < 0) {
            particle.velocity.z = -particle.velocity.z / (RAND + 1.5);
            particle.velocity.x = particle.velocity.x * (RAND + 0.5);
        }
    }
}

FountainEmitter::FountainEmitter(Drawable* _model, int number) : IntParticleEmitter(_model, number) {}

void FountainEmitter::createNewParticle(int index) {
    particleAttributes& particle = p_attributes[index];

    particle.position = emitter_pos;
    particle.velocity = glm::vec3(0, 0.1 + RAND * 0.1, 0);

    particle.mass = 0.01f + RAND * 0.03f;
    particle.rot_axis = glm::normalize(glm::vec3(2 - 2 * RAND, 4 - 2 * RAND, 2 - 2 * RAND));
    particle.rot_angle = RAND * 6.28;
    particle.life = 1.0f; //mark it alive
}

void FountainEmitter::updateParticles(float time, float dt) {

    //This is for the fountain to slowly increase the number of its particles to the max amount
    //instead of shooting all the particles at once
    if (active_particles < number_of_particles) {
        int batch = 5;
        int limit = std::min(number_of_particles - active_particles, batch);
        for (int i = 0; i < limit; i++) {
            createNewParticle(active_particles);
            active_particles++;
        }
    }
    else {
        active_particles = number_of_particles; //In case we resized our ermitter to a smaller particle number
    }

    for (int i = 0; i < active_particles; i++) {
        particleAttributes& particle = p_attributes[i];
       

        if (particle.position.y > emitter_pos.y + 2.25f )  createNewParticle(i);
        if (time == 0) {
            particle.mass = 0;
        }

        particle.rot_angle += 1.57 * dt;

        particle.position = function(particle.position) +
            glm::vec3(0.3 * particle.position.y * sin(particle.rot_angle), particle.velocity.y * dt, 0.3 * particle.position.y * cos(particle.rot_angle)); // +particle.accel * (dt * dt) * 0.5f;;
        particle.velocity = particle.velocity; 

    }
}

void FountainEmitter::createNewCoin(int index) {

    particleAttributes& particle = p_attributes[index];

    particle.position = emitter_pos; 
    particle.velocity = glm::vec3(3.5 - RAND * 7, - 2 - RAND * 0.5, 3.5 - RAND * 7);

    particle.mass = 2.0f;
    particle.rot_axis = glm::normalize(glm::vec3(4 - 8 * RAND, 4 - 8 * RAND, 4 - 8 * RAND));
    particle.rot_angle =RAND*360;
    particle.life = 1.0f; 
}

void FountainEmitter::updateCoins(float time, float dt) {

    if (active_particles < number_of_particles) {
        int batch = 2;
        int limit = std::min(number_of_particles - active_particles, batch);
        for (int i = 0; i < limit; i++) {
            createNewCoin(active_particles);
            active_particles++;
        }
    }
    else {
        active_particles = number_of_particles; 
    }

    for (int i = 0; i < active_particles; i++) {
        particleAttributes& particle = p_attributes[i];
        
        // check collision with table
        if (particle.life != 0.5 && particle.position.y < 0.4 && 
            particle.position.x <  2 && particle.position.x > -2 && 
            particle.position.z < 2.2 && particle.position.z > -2.2) {

            particle.rot_axis = glm::normalize(glm::vec3(1, 0, 0));
            if (RAND < 0.6 && particle.rot_angle != 180)  particle.rot_angle = 0;
            else particle.rot_angle = 180;
            particle.velocity = glm::vec3(0);
            particle.life = 0.5; //alive but motionless
        }

        // check collision with couch seat
        else if (particle.life != 0.5 && particle.position.y < 0.4 && particle.position.y > 0.33 &&
            particle.position.x <  7.2 && particle.position.x > 5.7 &&
            particle.position.z < 2.5 && particle.position.z > -2.3) {

            particle.rot_axis = glm::normalize(glm::vec3(1, 0, 0));
            if (RAND < 0.6 && particle.rot_angle != 180)  particle.rot_angle = 0;
            else particle.rot_angle = 180;
            particle.velocity = glm::vec3(0);
            particle.life = 0.5; //alive but motionless
        }

        // check collision with couch back
        else if (particle.life != 0.5 && particle.position.y < 2.0 && particle.position.y > 0.4 &&
            particle.position.x < 8 && particle.position.x > 7.3 &&
            particle.position.z < 3.4 && particle.position.z > -3.4) 
            particle.velocity += glm::vec3(-0.1 , 0.0, 0.0);
        
        // check collision with couch arm left
        else if (particle.life != 0.5 && particle.position.y < 1.7 &&
            particle.position.x < 7.55 && particle.position.x > 5.5 &&
            particle.position.z < 3.5 && particle.position.z > 2.5)
            particle.velocity += glm::vec3(0.0, 0.0, -0.1);
        
        // check collision with couch arm right
        else if (particle.life != 0.5 && particle.position.y < 1.7 &&
            particle.position.x < 7.55 && particle.position.x > 5.5 &&
            particle.position.z < -2.5 && particle.position.z > -3.5)
            particle.velocity += glm::vec3(0.0, 0.0, 0.1);

        // check collision with ground
        else if (particle.life != 0.5 && particle.position.y < -0.96 && particle.position.y > -1 ) {

            particle.rot_axis = glm::normalize(glm::vec3(1, 0, 0));
            if (RAND < 0.6 && particle.rot_angle!=180)  particle.rot_angle = 0;
            else particle.rot_angle = 180;
            particle.velocity = glm::vec3(0);
            particle.life = 0.5; //alive but motionless
        }

        if (particle.life!=0.5) {
            particle.rot_angle += 100 * dt;
            particle.position += particle.velocity * dt;
        }


        if (time == 0) particle.mass = 0;
        else particle.mass = 2.0;
      
    }
    if (dt == -1) active_particles = 0;
}