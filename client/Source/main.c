#include "config.h"
#include "hid.h"
#include "raylib.h"
#include "raymath.h"
#include "socketshit.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define SWIDTH 619
#define SHEIGHT 360

config cfg;
Font buttonFont;
Font fatFont;
int global = 0;
Color buttonColor, controllerColorLeft, controllerColorRight;

Model _controllerModel; // private

int countDigits(int value) {
  if (value < 0)
    value = -value;

  int count = 0;
  do {
    count++;
    value /= 10;
  } while (value != 0);

  return count;
}

void drawStick(Vector2 pos, HidAnalogStickState stick, HidNpadButton button, int materialIndex) {
  Vector2 at;
  at.x = pos.x + stick.x / 800;
  at.y = pos.y + -stick.y / 800;
  DrawRing(pos, 56, 50, 0, 360, 40, *(Color *)&cfg.colInactive);
  DrawCircleV(at, 40,
              getPacketData()->keys & button ? *(Color *)&cfg.colActive
                                             : *(Color *)&cfg.colStick);
  char buf[64];
  snprintf(buf, sizeof(buf), "%d", global);
  at.x -= countDigits(global) * 3;
  at.y -= 7;
  DrawTextEx(fatFont, buf, at, 9, 2, WHITE);

  // TODO: test
  if (materialIndex != -1) {
    _controllerModel.materials[materialIndex].maps[0].color = (getPacketData()->keys & button) != 0 
      ? *(Color *)&cfg.colActive
      : *(Color *)&cfg.colStick;
  }
}

Quaternion toQuaternion(float m[3][3]) {
  Quaternion q;

  float trace = m[0][0] + m[1][1] + m[2][2];
  if (trace > 0) {
    float s = 0.5f / sqrtf(trace + 1.0f);
    q.w = 0.25f / s;
    q.x = (m[2][1] - m[1][2]) * s;
    q.y = (m[0][2] - m[2][0]) * s;
    q.z = (m[1][0] - m[0][1]) * s;
  } else {
    if (m[0][0] > m[1][1] && m[0][0] > m[2][2]) {
      float s = 2.0f * sqrtf(1.0f + m[0][0] - m[1][1] - m[2][2]);
      q.w = (m[2][1] - m[1][2]) / s;
      q.x = 0.25f * s;
      q.y = (m[0][1] + m[1][0]) / s;
      q.z = (m[0][2] + m[2][0]) / s;
    } else if (m[1][1] > m[2][2]) {
      float s = 2.0f * sqrtf(1.0f + m[1][1] - m[0][0] - m[2][2]);
      q.w = (m[0][2] - m[2][0]) / s;
      q.x = (m[0][1] + m[1][0]) / s;
      q.y = 0.25f * s;
      q.z = (m[1][2] + m[2][1]) / s;
    } else {
      float s = 2.0f * sqrtf(1.0f + m[2][2] - m[0][0] - m[1][1]);
      q.w = (m[1][0] - m[0][1]) / s;
      q.x = (m[0][2] + m[2][0]) / s;
      q.y = (m[1][2] + m[2][1]) / s;
      q.z = 0.25f * s;
    }
  }

  return q;
}

Color getTextColor(Color backgroundColor)
{
  float luminosity = (backgroundColor.r * 0.299f + backgroundColor.g * 0.587f + backgroundColor.b * 0.114f) / 255.0f;
  return luminosity > 0.5f ? BLACK : WHITE;
}

void drawButton(Vector2 pos, float width, float height, HidNpadButton button,
                const char *label, float roundness, float fontOffsetX,
                float fontOffsetY, float fontSize, int materialIndex) {
  Rectangle rec;
  rec.x = pos.x - width;
  rec.y = pos.y - height;
  rec.height = height * 2;
  rec.width = width * 2;
  DrawRectangleRounded(rec, roundness, 8,
                       getPacketData()->keys & button
                           ? *(Color *)&cfg.colActive
                           : *(Color *)&cfg.colInactive);
  pos.x += fontOffsetX;
  pos.y += fontOffsetY;
  DrawTextEx(buttonFont, label, pos, fontSize, 4, cfg.useSystemButtonColor
              ? getTextColor(buttonColor)
              : *(Color *)&cfg.colFont);

  // TODO: test
  if (materialIndex != -1) {
    _controllerModel.materials[materialIndex].maps[0].color = (getPacketData()->keys & button) != 0 
      ? *(Color *)&cfg.colActive
      : *(Color *)&cfg.colInactive;
  }
}

float clamp(float num, float min, float max)
{
  if (num < min)
    num = min;
  else if (num > max)
    num = max;
  return num;
}

Color getStickColor(Color buttonColor)
{
  float luminosity = (buttonColor.r * 0.299f + buttonColor.g * 0.587f + buttonColor.b * 0.114f) / 255.0f;
  int adjustFactor = luminosity > 0.5f ? -65 : 65;

  buttonColor.r = (unsigned char)clamp(buttonColor.r + adjustFactor, 0, 255);
  buttonColor.g = (unsigned char)clamp(buttonColor.g + adjustFactor, 0, 255);
  buttonColor.b = (unsigned char)clamp(buttonColor.b + adjustFactor, 0, 255);
  return buttonColor;
}

int main() {
  cfg = loadConfig();
  InitWindow(SWIDTH, SHEIGHT, "HOS-InputDisplay");
  //SetConfigFlags(FLAG_MSAA_4X_HINT);

  buttonFont = LoadFontEx(cfg.fontPath, 38, 0, 250); // TODO: try 38, null, 0 ?
  fatFont = LoadFontEx(cfg.fatFontPath, 38, 0, 250);

  Camera3D camera = {0};
  camera.projection = CAMERA_PERSPECTIVE;
  camera.fovy = 45;
  camera.target.x = 0;
  camera.target.y = 0;
  camera.target.z = -6;
  camera.up.x = 0;
  camera.up.y = 1;
  camera.up.z = 0;
  camera.position.x = 0;
  camera.position.y = -15;
  camera.position.z = 15;
  _controllerModel = LoadModel("Res/Controller.glb");

  Vector3 controllerModelPos;
  controllerModelPos.x = 0;
  controllerModelPos.y = 0;
  controllerModelPos.z = 0;

  Quaternion slerpTo;
  slerpTo.x = 0;
  slerpTo.y = 0;
  slerpTo.z = 0;
  slerpTo.w = 1;
  Quaternion baseRotation;
  baseRotation.x = 0;
  baseRotation.y = 0;
  baseRotation.z = 0;
  baseRotation.w = 1;
  int cooldown = -1; // calibrationCooldown

  _controllerModel.materials[2].maps[0].color = *(Color *)&cfg.colController;
  _controllerModel.materials[7].maps[0].color = *(Color *)&cfg.colInactive;

  initSocketShit(cfg);

  while (1) {
    updateSocketShit();

    Quaternion quat = toQuaternion(getPacketData()->state.direction.direction);
    quat.x *= -1;
    quat.y *= -1;
    quat.z *= -1;
    _controllerModel.transform = QuaternionToMatrix(
        QuaternionMultiply(QuaternionInvert(baseRotation), quat));
    if (cooldown > 0) {
      cooldown--;

      baseRotation =
          QuaternionSlerp(baseRotation, slerpTo,
                          ((float)cfg.packetsPerSecond / 2 - (float)cooldown) /
                              (float)cfg.packetsPerSecond / 2);
    }
    if (getPacketData()->keys & HidNpadButton_StickL && cooldown <= 0) {
      cooldown = cfg.packetsPerSecond / 2;
      slerpTo = toQuaternion(getPacketData()->state.direction.direction);
      slerpTo.x *= -1;
      slerpTo.y *= -1;
      slerpTo.z *= -1;
    }

    controllerColorLeft = (Color) {((unsigned char)(getPacketData()->leftColor & 0xff),
                              (unsigned char)((getPacketData()->leftColor & 0xff00) >> 8),
                              (unsigned char)((getPacketData()->leftColor & 0xff0000) >> 0x10),
                              (unsigned char)((getPacketData()->leftColor & -16777216) >> 0x18)
                              )};

    controllerColorRight = (Color) {((unsigned char)(getPacketData()->rightColor & 0xff),
                              (unsigned char)((getPacketData()->rightColor & 0xff00) >> 8),
                              (unsigned char)((getPacketData()->rightColor & 0xff0000) >> 0x10),
                              (unsigned char)((getPacketData()->rightColor & -16777216) >> 0x18)
                              )};
                              
    if (cfg.useSystemButtonColor)
    {
      buttonColor = (Color) {((unsigned char)(getPacketData()->colorButton & 0xff),
                              (unsigned char)((getPacketData()->colorButton & 0xff00) >> 8),
                              (unsigned char)((getPacketData()->colorButton & 0xff0000) >> 0x10),
                              (unsigned char)((getPacketData()->colorButton & -16777216) >> 0x18)
                              )};
      cfg.colInactive = (*(CColor *)&buttonColor);
      Color colStick = getStickColor(buttonColor);
      cfg.colStick = (*(CColor *)&colStick);
    }


    BeginDrawing();
    ClearBackground(*(Color *)&cfg.colBg);

    if ((getPacketData()->styleSet & (int)(HidNpadStyleTag_JoyDual)) != 0)
    {
    }
    else if ((getPacketData()->styleSet & (int)(HidNpadStyleTag_FullKey)) != 0 && cfg.enableGyroModels)
    {
      if (cfg.useSystemControllerColor)
        _controllerModel.materials[2].maps[0].color = controllerColorLeft;
      _controllerModel.materials[7].maps[0].color = (*(Color *)&cfg.colInactive);
    }

    BeginMode3D(camera);
    DrawModel(_controllerModel, controllerModelPos, 2, WHITE);
    EndMode3D();

    Vector2 pos;
    pos.x = 100;
    pos.y = 180;
    drawStick(pos, getPacketData()->lPos, HidNpadButton_StickL, 8);
    pos.x = 380;
    pos.y = 270;
    drawStick(pos, getPacketData()->rPos, HidNpadButton_StickR, 9);
    pos.x = 550;
    pos.y = 180;
    drawButton(pos, 25, 25, HidNpadButton_A, "A", 1, -12, -19, 38, 10);
    pos.x = 450;
    pos.y = 180;
    drawButton(pos, 25, 25, HidNpadButton_Y, "Y", 1, -12, -19, 38, 13);
    pos.x = 500;
    pos.y = 230;
    drawButton(pos, 25, 25, HidNpadButton_B, "B", 1, -12, -19, 38, 12);
    pos.x = 500;
    pos.y = 130;
    drawButton(pos, 25, 25, HidNpadButton_X, "X", 1, -12, -19, 38, 11);
    pos.x = 380;
    pos.y = 130;
    drawButton(pos, 12, 12, HidNpadButton_Plus, "+", 1, -9, -19, 38, 14);
    pos.x = 220;
    pos.y = 130;
    drawButton(pos, 12, 12, HidNpadButton_Minus, "-", 1, -7, -21, 38, 15);
    pos.x = 500;
    pos.y = 70;
    drawButton(pos, 40, 15, HidNpadButton_R, "R", 1, -12, -20, 38, 18);
    pos.x = 500;
    pos.y = 30;
    drawButton(pos, 40, 15, HidNpadButton_ZR, "ZR", 1, -22, -19, 38, 16);
    pos.x = 100;
    pos.y = 70;
    drawButton(pos, 40, 15, HidNpadButton_L, "L", 1, -12, -20, 38, 17);
    pos.x = 100;
    pos.y = 30;
    drawButton(pos, 40, 15, HidNpadButton_ZL, "ZL", 1, -22, -19, 38, 1);
    pos.x = 220;
    pos.y = 240;
    drawButton(pos, 15, 15, HidNpadButton_Up, "", .5, 0, 0, 38, 3);
    pos.x = 220;
    pos.y = 300;
    drawButton(pos, 15, 15, HidNpadButton_Down, "", .5, 0, 0, 38, 5);
    pos.x = 190;
    pos.y = 270;
    drawButton(pos, 15, 15, HidNpadButton_Left, "", .5, 0, 0, 38, 6);
    pos.x = 250;
    pos.y = 270;
    drawButton(pos, 15, 15, HidNpadButton_Right, "", .5, 0, 0, 38, 4);

    pos.x = 220;
    pos.y = 270;
    Rectangle rec;
    rec.x = pos.x - 20; // 200
    rec.y = pos.y - 15; // 255
    rec.height = 15 * 2;
    rec.width = 20 * 2;
    DrawRectangleRounded(rec, 0, 8, *(Color *)&cfg.colInactive);
    rec.x = pos.x - 15; // 205
    rec.y = pos.y - 20; // 250
    rec.height = 20 * 2;
    rec.width = 15 * 2;
    DrawRectangleRounded(rec, 0, 8, *(Color *)&cfg.colInactive);

    if (cooldown > 0) {
      pos.x = 180;
      pos.y = 10;
      DrawTextEx(buttonFont, "Recalibrating", pos, 38, 4, WHITE);
    }

    EndDrawing();

    global++;
  }

  free((void *)cfg.host);
  free((void *)cfg.fontPath);
  return 0;
}